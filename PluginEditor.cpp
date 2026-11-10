#include "PluginEditor.h"
#include <cmath>

namespace token
{
namespace
{
    juce::Image makeGrainTexture()
    {
        juce::Image img (juce::Image::ARGB, 256, 256, true, juce::SoftwareImageType());
        juce::Random r (7);
        {
            juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < img.getHeight(); ++y)
                for (int x = 0; x < img.getWidth(); ++x)
                    data.setPixelColour (x, y, (r.nextBool() ? juce::Colours::white : juce::Colours::black)
                                                   .withAlpha (r.nextFloat() * 0.6f));
        }
        return juce::NativeImageType().convert (img);
    }

    bool isOn (const std::atomic<float>* p) { return p != nullptr && p->load() > 0.5f; }

    const char* visualName (VisualMode m)
    {
        switch (m)
        {
            case VisualMode::Classic: return "CLASSIC";
            case VisualMode::Orbit:   return "ORBIT";
            case VisualMode::Mirror:  return "MIRROR";
            case VisualMode::None:    return "SPECTRUM";
        }
        return "";
    }
}

// =================================================================================================
TokenEditor::TokenEditor (TokenAudioProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      visualSelector (*p.apvts.getParameter (ids::visual), { "CLASSIC", "ORBIT", "MIRROR", "NONE" }),
      colourSelector (*p.apvts.getParameter (ids::colour), { "MANUAL", "BEAT COLOUR" }),
      themeSelector  (*p.apvts.getParameter (ids::theme),  { "DARK", "LIGHT" }),
      layoutSelector (*p.apvts.getParameter (ids::mode),   { "STRIP", "LARGE" }),
      cleanToggle    (*p.apvts.getParameter (ids::clean),  "CLEAN")
{
    pMode      = p.apvts.getRawParameterValue (ids::mode);
    pVisual    = p.apvts.getRawParameterValue (ids::visual);
    pTheme     = p.apvts.getRawParameterValue (ids::theme);
    pColour    = p.apvts.getRawParameterValue (ids::colour);
    pIntensity = p.apvts.getRawParameterValue (ids::intensity);
    pClean     = p.apvts.getRawParameterValue (ids::clean);

    look::setActive (lightTheme());
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    intensitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    intensitySlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    intensitySlider.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, ids::intensity, intensitySlider);
    intensitySlider.setDoubleClickReturnValue (true, defaults::intensity);

    for (auto* c : std::initializer_list<juce::Component*> { &visualSelector, &colourSelector, &themeSelector,
                                                             &layoutSelector, &cleanToggle, &intensitySlider })
        addAndMakeVisible (c);

    pullLeft.resize ((size_t) ScopeFifo::capacity);
    pullRight.resize ((size_t) ScopeFifo::capacity);
    grainTexture = makeGrainTexture();

    clean = isOn (pClean);
    applyingMode = true;
    setResizable (true, true);
    applyMode (isOn (pMode) ? DisplayMode::Box : DisplayMode::Strip, true);

    startTime = lastTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
    vblank = std::make_unique<juce::VBlankAttachment> (this, std::function<void()> ([this] { frame(); }));
}

TokenEditor::~TokenEditor()
{
    vblank.reset();
    setLookAndFeel (nullptr);
}

bool TokenEditor::lightTheme() const
{
    return pTheme != nullptr && pTheme->load() > 0.5f;
}

// =================================================================================================
//  Layout
// =================================================================================================
void TokenEditor::applyMode (DisplayMode newMode, bool force)
{
    if (newMode == mode && ! force)
        return;

    mode = newMode;
    applyingMode = true;

    if (mode == DisplayMode::Strip)
        setResizeLimits (600, 140, 3200, 600);
    else
        setResizeLimits (640, 420, 3200, 2200);

    const int w = mode == DisplayMode::Strip ? proc.stripW.load() : proc.boxW.load();
    const int h = mode == DisplayMode::Strip ? proc.stripH.load() : proc.boxH.load();
    setSize (w, h);

    applyingMode = false;
    updateVisibility();
    resized();
}

void TokenEditor::updateVisibility()
{
    for (auto* c : std::initializer_list<juce::Component*> { &visualSelector, &colourSelector, &themeSelector,
                                                             &layoutSelector, &intensitySlider })
        c->setVisible (! clean);

    cleanToggle.setVisible (true);
    cleanToggle.setGhost (clean);
}

void TokenEditor::resized()
{
    if (! applyingMode && getWidth() > 0 && getHeight() > 0)
    {
        if (mode == DisplayMode::Strip) { proc.stripW = getWidth(); proc.stripH = getHeight(); }
        else                            { proc.boxW   = getWidth(); proc.boxH   = getHeight(); }
    }

    captions.clear();

    if (clean)                           layoutClean();
    else if (mode == DisplayMode::Strip) layoutStrip();
    else                                 layoutBox();
}

void TokenEditor::layoutBox()
{
    auto b = getLocalBounds().toFloat();
    const float m = 14.0f;

    barRect = b.removeFromBottom (68.0f);
    screenRect = b.withTrimmedTop (m).withTrimmedLeft (m).withTrimmedRight (m).withTrimmedBottom (2.0f);

    auto inner = barRect.reduced (m + 4.0f, 12.0f);
    brandRect = inner.removeFromLeft (juce::jlimit (110.0f, 190.0f, inner.getWidth() * 0.2f));
    inner.removeFromLeft (12.0f);

    struct Slot { juce::Component* comp; const char* caption; float weight; };
    const Slot slots[] = {
        { &visualSelector,  "VISUAL",    4.3f },
        { &colourSelector,  "COLOUR",    2.6f },
        { &themeSelector,   "THEME",     1.9f },
        { &intensitySlider, "INTENSITY", 2.7f },
        { &layoutSelector,  "LAYOUT",    1.9f },
        { &cleanToggle,     "VIEW",      1.3f },
    };

    const float gap = 12.0f;
    float totalWeight = 0.0f;
    for (const auto& s : slots)
        totalWeight += s.weight;

    const int numSlots = (int) (sizeof (slots) / sizeof (slots[0]));
    const float available = inner.getWidth() - gap * (float) (numSlots - 1);
    const float controlH = 22.0f;

    for (const auto& s : slots)
    {
        auto col = inner.removeFromLeft (available * s.weight / totalWeight);
        inner.removeFromLeft (gap);
        captions.push_back ({ s.caption, col.removeFromTop (14.0f) });
        s.comp->setBounds (col.withSizeKeepingCentre (col.getWidth(), controlH).toNearestInt());
    }
}

void TokenEditor::layoutStrip()
{
    auto b = getLocalBounds().toFloat();
    const float m = 12.0f;

    const float panelW = juce::jlimit (230.0f, 290.0f, b.getWidth() * 0.26f);
    barRect = b.removeFromRight (panelW);
    screenRect = b.reduced (m).withTrimmedRight (-4.0f);

    auto inner = barRect.reduced (12.0f, 10.0f);
    brandRect = inner.removeFromTop (juce::jmin (28.0f, inner.getHeight() * 0.2f));
    inner.removeFromTop (4.0f);

    const float gap = 6.0f;
    const float rowH = juce::jlimit (16.0f, 24.0f, (inner.getHeight() - gap * 3.0f) / 4.0f);
    auto row = [&] { auto r = inner.removeFromTop (rowH); inner.removeFromTop (gap); return r; };

    visualSelector.setBounds (row().toNearestInt());

    auto r2 = row();
    colourSelector.setBounds (r2.removeFromLeft (r2.getWidth() * 0.58f).toNearestInt());
    r2.removeFromLeft (6.0f);
    themeSelector.setBounds (r2.toNearestInt());

    auto r3 = row();
    captions.push_back ({ "INTENSITY", r3.removeFromLeft (64.0f) });
    intensitySlider.setBounds (r3.toNearestInt());

    auto r4 = row();
    layoutSelector.setBounds (r4.removeFromLeft (r4.getWidth() * 0.6f).toNearestInt());
    r4.removeFromLeft (6.0f);
    cleanToggle.setBounds (r4.toNearestInt());
}

void TokenEditor::layoutClean()
{
    screenRect = getLocalBounds().toFloat().reduced (3.0f);
    brandRect = barRect = {};
    cleanToggle.setBounds (getWidth() - 32, 8, 24, 24);
}

juce::Rectangle<float> TokenEditor::visualArea() const
{
    const auto s = screenRect;
    if (mode == DisplayMode::Strip)
        return s.reduced (s.getWidth() * 0.015f, s.getHeight() * 0.1f);
    return s.reduced (s.getWidth() * 0.03f, s.getHeight() * 0.1f);
}

void TokenEditor::mouseDoubleClick (const juce::MouseEvent&)
{
    if (auto* param = proc.apvts.getParameter (ids::clean))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (clean ? 0.0f : 1.0f);
        param->endChangeGesture();
    }
}

// =================================================================================================
//  Frame loop (message thread, synced to the display's vertical blank)
// =================================================================================================
Transport TokenEditor::readTransport (double now) const
{
    Transport t;
    t.bpm = juce::jlimit (20.0, 400.0, proc.hostBpm.load());
    t.playing = proc.hostPlaying.load();
    t.beatsPerBar = proc.hostBeatsPerBar.load();
    t.ppq = proc.hostPpq.load();

    if (t.playing)   // extrapolate between audio blocks for smooth motion
        t.ppq += juce::jlimit (0.0, 0.25, now - proc.hostPpqTime.load()) * t.bpm / 60.0;

    return t;
}

void TokenEditor::frame()
{
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt = juce::jlimit (1.0f / 240.0f, 0.1f, (float) (now - lastTime));
    lastTime = now;
    time = (float) (now - startTime);

    // layout-changing parameters (may come from host automation)
    const bool newClean = isOn (pClean);
    if (newClean != clean)
    {
        clean = newClean;
        updateVisibility();
        resized();
    }

    const auto newMode = isOn (pMode) ? DisplayMode::Box : DisplayMode::Strip;
    if (newMode != mode)
        applyMode (newMode, false);

    // audio from the processor (lock-free FIFO)
    const int n = proc.scopeFifo.pull (pullLeft.data(), pullRight.data(), (int) pullLeft.size());
    if (n > 0)
        history.append (pullLeft.data(), pullRight.data(), n);

    const double sr = proc.currentSampleRate.load();
    analyser.process (history, n, sr, dt);
    const auto& f = analyser.features();

    transport = readTransport (now);
    const bool light = lightTheme();
    colours.update (dt, f, transport, isOn (pColour), light);

    visualSettings.mode = (VisualMode) juce::jlimit (0, 3, juce::roundToInt (pVisual->load()));
    visualSettings.intensity = pIntensity->load();
    visualSettings.strip = mode == DisplayMode::Strip;
    visualiser.update (history, f, visualSettings, visualArea(), sr, dt, time);

    hasFrame = true;
    repaint();
}

// =================================================================================================
//  Painting
// =================================================================================================
void TokenEditor::paint (juce::Graphics& g)
{
    const bool light = lightTheme();
    look::setActive (light);                     // shared by all TOTEM windows: set before anything paints
    look::setAccent (colours.palette().a);
    const auto& lk = look::active();
    const auto b = getLocalBounds().toFloat();

    // background
    juce::ColourGradient bg (lk.chassisTop, 0.0f, 0.0f, lk.chassisBottom, 0.0f, b.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRect (b);

    paintScreen (g, lk);

    if (clean)
        return;

    // control area: a single hairline separates it from the artwork
    g.setColour (lk.separator);
    if (mode == DisplayMode::Box)
        g.fillRect (barRect.getX() + 14.0f, barRect.getY(), barRect.getWidth() - 28.0f, 1.0f);
    else
        g.fillRect (barRect.getX(), barRect.getY() + 12.0f, 1.0f, barRect.getHeight() - 24.0f);

    paintWordmark (g, lk, brandRect, mode == DisplayMode::Strip);

    g.setFont (ui::font (8.5f, "SemiBold", 0.3f));
    g.setColour (lk.caption);
    for (const auto& c : captions)
        g.drawText (c.text, c.area, juce::Justification::centredLeft, false);
}

void TokenEditor::paintScreen (juce::Graphics& g, const look::Look& lk)
{
    const auto s = screenRect;
    const float corner = clean ? 4.0f : 10.0f;
    const auto& pal = colours.palette();

    juce::Graphics::ScopedSaveState state (g);
    juce::Path clip;
    clip.addRoundedRectangle (s, corner);
    g.reduceClipRegion (clip);

    juce::ColourGradient surface (lk.screenTop, 0.0f, s.getY(), lk.screenBottom, 0.0f, s.getBottom(), false);
    g.setGradientFill (surface);
    g.fillRect (s);

    if (! hasFrame)
        return;

    const auto& f = analyser.features();
    const auto va = visualArea();

    // hairline graticule
    g.setColour (lk.grid);
    if (visualSettings.mode == VisualMode::Orbit || visualSettings.mode == VisualMode::Mirror)
    {
        g.fillRect (s.getX(), va.getCentreY(), s.getWidth(), 1.0f);
        g.fillRect (va.getCentreX(), s.getY(), 1.0f, s.getHeight());
    }
    else
    {
        g.fillRect (s.getX(), va.getCentreY(), s.getWidth(), 1.0f);
        for (int i = 1; i < 10; ++i)
            g.fillRect (va.getX() + va.getWidth() * (float) i / 10.0f, s.getY(), 1.0f, s.getHeight());
    }

    // spectrum: the whole stage when the oscilloscope is hidden, otherwise a quiet floor
    if (visualSettings.mode == VisualMode::None)
        visualiser.drawSpectrum (g, f, s.reduced (s.getWidth() * 0.04f, s.getHeight() * 0.14f), lk, pal, true);
    else
        visualiser.drawSpectrum (g, f, mode == DisplayMode::Strip ? s.reduced (4.0f, 0.0f)
                                                                  : s.withTop (s.getBottom() - s.getHeight() * 0.22f).reduced (8.0f, 0.0f),
                                 lk, pal, false);

    visualiser.draw (g, lk, pal, colours.pulse());

    // finishing: fine grain, soft vignette (dark), glass hairline
    g.setTiledImageFill (grainTexture, fxRandom.nextInt (256), fxRandom.nextInt (256), lk.light ? 0.025f : 0.035f);
    g.fillRect (s);

    if (! lk.light)
    {
        const auto c = s.getCentre();
        juce::ColourGradient vig (juce::Colours::transparentBlack, c.x, c.y,
                                  juce::Colours::black.withAlpha (0.45f), c.x + s.getWidth() * 0.62f, c.y, true);
        juce::FillType vf (vig);
        vf.transform = juce::AffineTransform::scale (1.0f, s.getHeight() / juce::jmax (1.0f, s.getWidth()) * 1.3f, c.x, c.y);
        g.setFillType (vf);
        g.fillRect (s);
    }

    g.setColour (juce::Colours::white.withAlpha (lk.light ? 0.5f : 0.05f));
    g.fillRect (s.getX() + corner, s.getY() + 1.0f, s.getWidth() - corner * 2.0f, 1.0f);

    paintInfo (g, lk);

    g.setColour (lk.frame);
    g.drawRoundedRectangle (s.reduced (0.5f), corner, 1.0f);
}

void TokenEditor::paintInfo (juce::Graphics& g, const look::Look& lk)
{
    auto s = screenRect.reduced (14.0f, 10.0f);

    if (clean)
    {
        // discreet signature for screen recordings
        auto r = s.withTop (s.getBottom() - 12.0f);
        g.setFont (ui::font (9.0f, "SemiBold", 0.45f));
        g.setColour (lk.textDim.withAlpha (0.55f));
        g.drawText ("SMSW  TOTEM", r, juce::Justification::centredRight, false);
        return;
    }

    auto r = s.removeFromTop (12.0f);
    g.setFont (ui::font (8.5f, "SemiBold", 0.3f));
    g.setColour (lk.textDim.withAlpha (0.8f));
    g.drawText (visualName (visualSettings.mode), r, juce::Justification::centredLeft, false);

    if (isOn (pColour))
    {
        const juce::String bpm = juce::String (transport.bpm, 1) + " BPM";
        const float pulse = colours.pulse();
        auto right = r;
        const auto dot = right.removeFromRight (8.0f).withSizeKeepingCentre (5.0f, 5.0f);
        g.setColour (colours.palette().a.withAlpha (0.35f + 0.65f * juce::jlimit (0.0f, 1.0f, pulse * 2.0f)));
        g.fillEllipse (dot);
        right.removeFromRight (6.0f);
        g.setColour (lk.textDim.withAlpha (0.8f));
        g.drawText ("BEAT COLOUR   " + bpm, right, juce::Justification::centredRight, false);
    }
}

void TokenEditor::paintWordmark (juce::Graphics& g, const look::Look& lk, juce::Rectangle<float> r, bool compact)
{
    if (r.isEmpty())
        return;

    if (compact)
    {
        const float h = juce::jlimit (14.0f, 20.0f, r.getHeight() * 0.75f);
        g.setFont (ui::font (h, "Light", 0.42f));
        g.setColour (lk.text);
        g.drawText ("TOTEM", r, juce::Justification::centredLeft, false);
        g.setFont (ui::font (8.5f, "SemiBold", 0.4f));
        g.setColour (lk.textDim);
        g.drawText ("SMSW", r, juce::Justification::centredRight, false);
        return;
    }

    const float h = juce::jlimit (16.0f, 26.0f, r.getHeight() * 0.55f);
    auto top = r.removeFromTop (r.getHeight() * 0.36f);
    g.setFont (ui::font (8.5f, "SemiBold", 0.45f));
    g.setColour (lk.textDim);
    g.drawText ("SMSW", top, juce::Justification::bottomLeft, false);

    g.setFont (ui::font (h, "Light", 0.42f));
    g.setColour (lk.text);
    g.drawText ("TOTEM", r, juce::Justification::centredLeft, false);

    // a short accent hairline under the wordmark
    g.setColour (look::accent().withAlpha (0.8f));
    g.fillRect (r.getX() + 1.0f, r.getCentreY() + h * 0.62f, 18.0f, 1.0f);
}
}

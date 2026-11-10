#include "PluginEditor.h"
#include "Util.h"
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
      cleanToggle    (*p.apvts.getParameter (ids::clean),  "CLEAN"),
      displaySelector (*p.apvts.getParameter (ids::artMode), { "SCOPE+ART", "ART", "SCOPE" }),
      lockToggle     (*p.apvts.getParameter (ids::lock),   "LOCK")
{
    pArt       = p.apvts.getRawParameterValue (ids::artMode);
    pSpeed     = p.apvts.getRawParameterValue (ids::speed);
    pLock      = p.apvts.getRawParameterValue (ids::lock);
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

    speedSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    speedSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    speedSlider.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, ids::speed, speedSlider);
    speedSlider.setDoubleClickReturnValue (true, defaults::speed);

    for (auto* c : std::initializer_list<juce::Component*> { &visualSelector, &colourSelector, &themeSelector,
                                                             &layoutSelector, &cleanToggle, &intensitySlider, &displaySelector,
                                                             &lockToggle, &speedSlider })
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
                                                             &layoutSelector, &intensitySlider, &displaySelector,
                                                             &lockToggle, &speedSlider })
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

void TokenEditor::placeRow (juce::Rectangle<float> row, std::initializer_list<std::pair<juce::Component*, const char*>> items,
                            std::initializer_list<float> weights, bool captionsAbove)
{
    const float gap = 10.0f;
    float total = 0.0f;
    for (float w : weights)
        total += w;

    const float available = row.getWidth() - gap * (float) ((int) items.size() - 1);
    auto w = weights.begin();
    for (const auto& item : items)
    {
        auto cell = row.removeFromLeft (available * (*w) / juce::jmax (0.01f, total));
        row.removeFromLeft (gap);
        ++w;

        if (captionsAbove && item.second != nullptr)
        {
            captions.push_back ({ item.second, cell.removeFromTop (13.0f) });
            item.first->setBounds (cell.withSizeKeepingCentre (cell.getWidth(), juce::jmin (21.0f, cell.getHeight())).toNearestInt());
        }
        else
        {
            if (item.second != nullptr)
                captions.push_back ({ item.second, cell.removeFromLeft (juce::jmin (62.0f, cell.getWidth() * 0.4f)) });
            item.first->setBounds (cell.toNearestInt());
        }
    }
}

void TokenEditor::layoutBox()
{
    auto b = getLocalBounds().toFloat();
    const float m = 14.0f;

    // two rows of controls under the screen; every size derives from the actual editor bounds
    barRect = b.removeFromBottom (juce::jlimit (88.0f, 104.0f, b.getHeight() * 0.15f));
    screenRect = b.withTrimmedTop (m).withTrimmedLeft (m).withTrimmedRight (m).withTrimmedBottom (2.0f);

    auto inner = barRect.reduced (m + 4.0f, 8.0f);
    brandRect = inner.removeFromLeft (juce::jlimit (100.0f, 190.0f, inner.getWidth() * 0.18f));
    inner.removeFromLeft (12.0f);

    auto row1 = inner.removeFromTop (inner.getHeight() * 0.5f);
    auto row2 = inner;

    placeRow (row1, { { &displaySelector, "DISPLAY" }, { &visualSelector, "VISUAL" }, { &colourSelector, "COLOUR" }, { &themeSelector, "THEME" } },
              { 3.2f, 4.3f, 2.8f, 2.0f }, true);
    placeRow (row2, { { &intensitySlider, "INTENSITY" }, { &speedSlider, "SPEED" }, { &lockToggle, "SCENE" },
                      { &layoutSelector, "LAYOUT" }, { &cleanToggle, "VIEW" } },
              { 3.0f, 3.0f, 1.3f, 2.0f, 1.3f }, true);
}

void TokenEditor::layoutStrip()
{
    auto b = getLocalBounds().toFloat();
    const float m = 10.0f;

    // shallow windows get a wider two-column panel so nothing is squeezed or clipped
    const bool twoColumns = b.getHeight() < 175.0f;
    const float panelW = twoColumns ? juce::jlimit (330.0f, 460.0f, b.getWidth() * 0.4f)
                                    : juce::jlimit (210.0f, 300.0f, b.getWidth() * 0.27f);
    barRect = b.removeFromRight (panelW);
    screenRect = b.reduced (m).withTrimmedRight (-4.0f);

    auto inner = barRect.reduced (10.0f, 8.0f);
    const float gap = 4.0f;

    if (! twoColumns && inner.getHeight() >= 6.0f * 16.0f + 5.0f * gap + 22.0f)
    {
        brandRect = inner.removeFromTop (20.0f);
        inner.removeFromTop (4.0f);
    }
    else
    {
        brandRect = {};
    }

    auto layoutColumn = [gap] (juce::Rectangle<float> col, int rows) -> std::vector<juce::Rectangle<float>>
    {
        std::vector<juce::Rectangle<float>> out;
        const float rowH = juce::jlimit (12.0f, 22.0f, (col.getHeight() - gap * (float) (rows - 1)) / (float) rows);
        for (int i = 0; i < rows; ++i)
        {
            out.push_back (col.removeFromTop (rowH));
            col.removeFromTop (gap);
        }
        return out;
    };

    std::vector<juce::Rectangle<float>> r;
    if (twoColumns)
    {
        auto left = inner.removeFromLeft (inner.getWidth() * 0.5f - 4.0f);
        inner.removeFromLeft (8.0f);
        const auto a = layoutColumn (left, 3);
        const auto c = layoutColumn (inner, 3);
        r = { a[0], a[1], a[2], c[0], c[1], c[2] };
    }
    else
    {
        r = layoutColumn (inner, 6);
    }

    displaySelector.setBounds (r[0].toNearestInt());
    visualSelector.setBounds (r[1].toNearestInt());
    placeRow (r[2], { { &colourSelector, nullptr }, { &themeSelector, nullptr } }, { 1.4f, 1.0f }, false);
    placeRow (r[3], { { &intensitySlider, "INTENSITY" } }, { 1.0f }, false);
    placeRow (r[4], { { &speedSlider, "SPEED" } }, { 1.0f }, false);
    placeRow (r[5], { { &layoutSelector, nullptr }, { &lockToggle, nullptr }, { &cleanToggle, nullptr } }, { 1.6f, 1.0f, 1.0f }, false);
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

juce::Rectangle<float> TokenEditor::artCanvas() const
{
    // a safe canvas inside the screen: clear of the edges and of the small readout at the top
    auto s = screenRect.reduced (screenRect.getWidth() * 0.02f, screenRect.getHeight() * 0.05f);
    if (! clean)
        s.removeFromTop (juce::jmin (16.0f, s.getHeight() * 0.1f));
    return s;
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
    visualSettings.speed = speedFactor (pSpeed->load());
    visualiser.update (history, f, visualSettings, visualArea(), sr, dt, time);

    // artwork: SCOPE + ART / ART ONLY / SCOPE ONLY (the scope fades rather than snapping away)
    artMode = (ArtMode) juce::jlimit (0, 2, juce::roundToInt (pArt->load()));
    scopeFade = smoothTo (scopeFade, artMode == ArtMode::ArtOnly ? 0.0f : 1.0f, dt, 0.35f);

    artContext.area = artCanvas();
    artContext.time = time;
    artContext.dt = dt;
    artContext.enabled = artMode != ArtMode::ScopeOnly;
    artContext.artOnly = artMode == ArtMode::ArtOnly;
    artContext.strip = mode == DisplayMode::Strip;
    artContext.locked = isOn (pLock);
    artContext.intensity = visualSettings.intensity;
    artContext.speed = visualSettings.speed;
    artContext.beats = colours.beats();
    artContext.beatsPerBar = colours.beatsPerBar();
    artContext.beatPhase = colours.beatPhase();
    artContext.barPhase = colours.barPhase();
    artContext.hostPlaying = colours.synced();
    artContext.audio = &f;
    artContext.palette = colours.palette();
    artContext.theme = light ? &look::light() : &look::dark();
    artEngine.update (artContext);

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

    // everything that belongs to the scope fades together (ART ONLY hides it completely)
    const bool scopeVisible = scopeFade > 0.01f;
    const bool fadingScope = scopeVisible && scopeFade < 0.99f;

    // hairline graticule
    if (scopeVisible)
    {
        g.setColour (lk.grid.withMultipliedAlpha (scopeFade));
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
    }

    // the artwork lives behind the signal. In SCOPE + ART it is a soft ghost everywhere and
    // shines through brightly wherever the waveform is - the picture seems to live inside the signal.
    if (artMode == ArtMode::ArtOnly || ! scopeVisible)
    {
        artEngine.draw (g, artContext, 1.0f);
    }
    else
    {
        const auto region = visualiser.revealRegion (juce::jmax (6.0f, s.getHeight() * 0.05f));
        if (region.isEmpty())
        {
            artEngine.draw (g, artContext, 0.8f);
        }
        else
        {
            artEngine.draw (g, artContext, 0.42f);
            juce::Graphics::ScopedSaveState revealState (g);
            g.reduceClipRegion (region);
            artEngine.draw (g, artContext, 0.9f);
        }
    }

    if (scopeVisible)
    {
        if (fadingScope)
            g.beginTransparencyLayer (scopeFade);

        // spectrum: the whole stage when the oscilloscope is hidden, otherwise a quiet floor
        if (visualSettings.mode == VisualMode::None)
            visualiser.drawSpectrum (g, f, s.reduced (s.getWidth() * 0.04f, s.getHeight() * 0.14f), lk, pal, true);
        else
            visualiser.drawSpectrum (g, f, mode == DisplayMode::Strip ? s.reduced (4.0f, 0.0f)
                                                                      : s.withTop (s.getBottom() - s.getHeight() * 0.22f).reduced (8.0f, 0.0f),
                                     lk, pal, false);

        visualiser.draw (g, lk, pal, colours.pulse());

        if (fadingScope)
            g.endTransparencyLayer();
    }

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
    juce::String modeText = artMode == ArtMode::ArtOnly ? juce::String ("ART")
                          : juce::String (visualName (visualSettings.mode)) + (artMode == ArtMode::ScopeAndArt ? "  +  ART" : "");
    if (artMode != ArtMode::ScopeOnly && isOn (pLock))
        modeText += "   /   LOCKED";
    g.drawText (modeText, r, juce::Justification::centredLeft, false);

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

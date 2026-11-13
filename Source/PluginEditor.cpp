#include "PluginEditor.h"
#include "Util.h"
#include <cmath>

namespace token
{
namespace
{
    bool isOn (const std::atomic<float>* p) { return p != nullptr && p->load() > 0.5f; }

    int choiceOf (const std::atomic<float>* p, int maxIndex)
    {
        return p == nullptr ? 0 : juce::jlimit (0, maxIndex, juce::roundToInt (p->load()));
    }

    constexpr float screenCorner = 10.0f;
    const juce::Colour darkScreen { 0xff030407 };
    const juce::Colour lightScreen { 0xfff4f0e7 };
}

// =================================================================================================
TokenEditor::TokenEditor (TokenAudioProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      colourSelector (*p.apvts.getParameter (ids::palette), { "ABYSS", "UV", "ACID", "BLOOD", "GOLD", "PULSE" }),
      themeSelector  (*p.apvts.getParameter (ids::theme),   { "DARK", "LIGHT" }),
      layoutSelector (*p.apvts.getParameter (ids::mode),    { "STRIP", "BOX" }),
      lockToggle     (*p.apvts.getParameter (ids::lock),    "LOCK"),
      cleanToggle    (*p.apvts.getParameter (ids::clean),   "CLEAN")
{
    pMode    = p.apvts.getRawParameterValue (ids::mode);
    pClean   = p.apvts.getRawParameterValue (ids::clean);
    pTheme   = p.apvts.getRawParameterValue (ids::theme);
    pPalette = p.apvts.getRawParameterValue (ids::palette);
    pReact   = p.apvts.getRawParameterValue (ids::react);
    pSpeed   = p.apvts.getRawParameterValue (ids::speed);
    pLock    = p.apvts.getRawParameterValue (ids::lock);

    lightTheme = isOn (pTheme);
    look::setActive (lightTheme);
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    for (auto* s : { &reactSlider, &speedSlider })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s->setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    reactAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, ids::react, reactSlider);
    speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, ids::speed, speedSlider);
    reactSlider.setDoubleClickReturnValue (true, defaults::react);
    speedSlider.setDoubleClickReturnValue (true, defaults::speed);

    for (auto* c : std::initializer_list<juce::Component*> { &colourSelector, &themeSelector, &layoutSelector, &lockToggle,
                                                             &cleanToggle, &reactSlider, &speedSlider })
        addAndMakeVisible (c);

    pullLeft.resize ((size_t) ScopeFifo::capacity);
    pullRight.resize ((size_t) ScopeFifo::capacity);

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
        setResizeLimits (600, 140, 3840, 700);
    else
        setResizeLimits (640, 420, 3840, 2400);

    const int w = mode == DisplayMode::Strip ? proc.stripW.load() : proc.boxW.load();
    const int h = mode == DisplayMode::Strip ? proc.stripH.load() : proc.boxH.load();
    setSize (w, h);

    applyingMode = false;
    updateVisibility();
    resized();
    repaint();
}

void TokenEditor::updateVisibility()
{
    for (auto* c : std::initializer_list<juce::Component*> { &colourSelector, &themeSelector, &layoutSelector, &lockToggle,
                                                             &reactSlider, &speedSlider })
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

    rebuildScreenCache();
    repaint();
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
                captions.push_back ({ item.second, cell.removeFromLeft (juce::jmin (50.0f, cell.getWidth() * 0.35f)) });
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
    brandRect = inner.removeFromLeft (juce::jlimit (100.0f, 170.0f, inner.getWidth() * 0.16f));
    inner.removeFromLeft (12.0f);

    auto row1 = inner.removeFromTop (inner.getHeight() * 0.5f);
    auto row2 = inner;

    placeRow (row1, { { &colourSelector, "COLOUR" }, { &themeSelector, "THEME" }, { &layoutSelector, "LAYOUT" } },
              { 5.6f, 2.2f, 2.2f }, true);
    placeRow (row2, { { &reactSlider, "REACT" }, { &speedSlider, "SPEED" }, { &lockToggle, "HOLD" }, { &cleanToggle, "VIEW" } },
              { 3.4f, 3.4f, 1.3f, 1.3f }, true);
}

void TokenEditor::layoutStrip()
{
    auto b = getLocalBounds().toFloat();
    const float m = 10.0f;

    // a real strip layout: the screen keeps the full height, controls sit in a side panel that
    // switches to two columns when the window is shallow - nothing is cropped or stretched
    const bool twoColumns = b.getHeight() < 180.0f;
    const float panelW = twoColumns ? juce::jlimit (360.0f, 480.0f, b.getWidth() * 0.42f)
                                    : juce::jlimit (240.0f, 320.0f, b.getWidth() * 0.28f);
    barRect = b.removeFromRight (panelW);
    screenRect = b.reduced (m).withTrimmedRight (-4.0f);

    auto inner = barRect.reduced (10.0f, 8.0f);
    const float gap = 4.0f;

    if (! twoColumns && inner.getHeight() >= 5.0f * 16.0f + 4.0f * gap + 24.0f)
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

    if (twoColumns)
    {
        auto left = inner.removeFromLeft (inner.getWidth() * 0.56f - 4.0f);
        inner.removeFromLeft (8.0f);
        const auto a = layoutColumn (left, 3);
        const auto c = layoutColumn (inner, 3);
        colourSelector.setBounds (a[0].toNearestInt());
        placeRow (a[1], { { &themeSelector, nullptr }, { &layoutSelector, nullptr } }, { 1.0f, 1.0f }, false);
        placeRow (a[2], { { &lockToggle, nullptr }, { &cleanToggle, nullptr } }, { 1.0f, 1.0f }, false);
        placeRow (c[0], { { &reactSlider, "REACT" } }, { 1.0f }, false);
        placeRow (c[1], { { &speedSlider, "SPEED" } }, { 1.0f }, false);
    }
    else
    {
        const auto r = layoutColumn (inner, 5);
        colourSelector.setBounds (r[0].toNearestInt());
        placeRow (r[1], { { &themeSelector, nullptr }, { &layoutSelector, nullptr } }, { 1.0f, 1.0f }, false);
        placeRow (r[2], { { &reactSlider, "REACT" } }, { 1.0f }, false);
        placeRow (r[3], { { &speedSlider, "SPEED" } }, { 1.0f }, false);
        placeRow (r[4], { { &lockToggle, nullptr }, { &cleanToggle, nullptr } }, { 1.0f, 1.0f }, false);
    }
}

void TokenEditor::layoutClean()
{
    screenRect = getLocalBounds().toFloat().reduced (3.0f);
    brandRect = barRect = {};
    cleanToggle.setBounds (getWidth() - 32, 8, 24, 24);
}

juce::Rectangle<float> TokenEditor::visualArea() const
{
    auto s = screenRect.reduced (screenRect.getWidth() * (mode == DisplayMode::Strip ? 0.012f : 0.025f),
                                 screenRect.getHeight() * 0.06f);
    if (! clean)
        s.removeFromTop (juce::jmin (14.0f, s.getHeight() * 0.08f));   // room for the status line
    return s;
}

void TokenEditor::rebuildScreenCache()
{
    const auto s = screenRect;
    const float corner = clean ? 4.0f : screenCorner;

    // corner masks: the screen is clipped with a cheap rectangle, the rounded corners are
    // painted over in the chassis colour
    screenCorners.clear();
    screenCorners.setUsingNonZeroWinding (false);
    screenCorners.addRectangle (s);
    screenCorners.addRoundedRectangle (s, corner);

    // vignette (dark theme): rendered once per size instead of a full-screen gradient every frame
    const int w = juce::jmax (1, (int) s.getWidth()), h = juce::jmax (1, (int) s.getHeight());
    vignette = juce::Image (juce::Image::ARGB, w, h, true);
    {
        juce::Graphics vg (vignette);
        const juce::Point<float> c ((float) w * 0.5f, (float) h * 0.5f);
        juce::ColourGradient grad (juce::Colours::transparentBlack, c.x, c.y,
                                   juce::Colours::black.withAlpha (0.5f), c.x + (float) w * 0.62f, c.y, true);
        juce::FillType ft (grad);
        ft.transform = juce::AffineTransform::scale (1.0f, (float) h / (float) w * 1.3f, c.x, c.y);
        vg.setFillType (ft);
        vg.fillAll();
    }
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

void TokenEditor::adaptQuality (double paintMs)
{
    // graceful degradation: if drawing gets expensive, step quality down quickly;
    // step back up only after several seconds of comfortable frames
    paintAvgMs += (paintMs - paintAvgMs) * 0.1;

    if (paintAvgMs > 7.0 && quality > 0)
    {
        --quality;
        paintAvgMs = 3.0;
        goodTime = 0.0;
    }
    else if (paintAvgMs < 2.5 && quality < 2)
    {
        goodTime += 1.0 / 60.0;
        if (goodTime > 5.0)
        {
            ++quality;
            goodTime = 0.0;
        }
    }
    else
    {
        goodTime = 0.0;
    }
}

void TokenEditor::frame()
{
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt = juce::jlimit (1.0f / 240.0f, 0.1f, (float) (now - lastTime));
    lastTime = now;
    time = (float) (now - startTime);

    // layout / theme parameters (may come from host automation)
    bool chromeDirty = false;

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

    const bool newLight = isOn (pTheme);
    if (newLight != lightTheme)
    {
        lightTheme = newLight;
        look::setActive (lightTheme);
        chromeDirty = true;
    }

    // audio from the processor (lock-free FIFO; the audio thread only copies samples)
    const int n = proc.scopeFifo.pull (pullLeft.data(), pullRight.data(), (int) pullLeft.size());
    if (n > 0)
        history.append (pullLeft.data(), pullRight.data(), n);

    const double sr = proc.currentSampleRate.load();
    analyser.process (history, n, sr, dt);
    const auto& f = analyser.features();

    transport = readTransport (now);
    const int paletteIndex = choiceOf (pPalette, numPaletteChoices - 1);
    colours.update (dt, f, transport, paletteIndex);

    settings.speed = speedFactor (pSpeed->load());
    settings.react = pReact->load();
    settings.locked = isOn (pLock);
    settings.strip = mode == DisplayMode::Strip;
    settings.light = lightTheme;
    settings.quality = quality;
    totem.update (history, f, colours, settings, visualArea(), sr, dt, time);

    hasFrame = true;

    // the controls' accent follows the palette; repaint them only when it actually changes
    auto accent = paletteIndex < look::numStudioPalettes
                      ? look::studioPalette (paletteIndex).a
                      : look::studioPalette (juce::jlimit (0, look::numStudioPalettes - 1,
                                                           (int) std::fmod (std::abs (colours.beats()) / 8.0, (double) look::numStudioPalettes))).a;
    if (lightTheme)
        accent = look::studioPalette (paletteIndex < look::numStudioPalettes ? paletteIndex : 0).b.darker (0.3f);

    if (paletteIndex != lastPalette || accent != lastAccent)
    {
        lastPalette = paletteIndex;
        lastAccent = accent;
        look::setAccent (accent);
        chromeDirty = true;
    }

    if (chromeDirty)
        repaint();
    else
        repaint (screenRect.toNearestInt().expanded (1));
}

// =================================================================================================
//  Painting
// =================================================================================================
void TokenEditor::paint (juce::Graphics& g)
{
    look::setActive (lightTheme);
    const auto& lk = look::active();

    const auto clip = g.getClipBounds().toFloat();
    if (! screenRect.contains (clip))
        paintChrome (g, lk);

    if (clip.intersects (screenRect))
        paintScreen (g, lk);
}

void TokenEditor::paintChrome (juce::Graphics& g, const look::Look& lk)
{
    g.fillAll (lk.chassisBottom);

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
    const float corner = clean ? 4.0f : screenCorner;

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (s.toNearestInt());   // rectangle clip only: cheap on every renderer

        g.setColour (lightTheme ? lightScreen : darkScreen);
        g.fillRect (s);

        if (hasFrame)
        {
            const double t0 = juce::Time::getMillisecondCounterHiRes();
            totem.draw (g);
            adaptQuality (juce::Time::getMillisecondCounterHiRes() - t0);
        }

        if (! lightTheme && quality >= 1 && vignette.isValid())
            g.drawImageAt (vignette, (int) s.getX(), (int) s.getY());

        paintHud (g, lk);
    }

    g.setColour (lk.chassisBottom);
    g.fillPath (screenCorners);
    g.setColour (lk.frame);
    g.drawRoundedRectangle (s.reduced (0.5f), corner, 1.0f);
}

void TokenEditor::paintHud (juce::Graphics& g, const look::Look& lk)
{
    auto s = screenRect.reduced (14.0f, 9.0f);

    if (clean)
    {
        // discreet signature for screen recordings and projection
        auto r = s.withTop (s.getBottom() - 12.0f).withTrimmedRight (96.0f);
        g.setFont (ui::font (9.0f, "SemiBold", 0.45f));
        g.setColour (lk.textDim.withAlpha (0.4f));
        g.drawText ("SMSW  TOTEM 2", r, juce::Justification::centredLeft, false);
        return;
    }

    auto r = s.removeFromTop (12.0f);
    g.setFont (ui::font (8.5f, "SemiBold", 0.3f));

    // left: the image that is on screen, and its number in the library
    juce::String status = juce::String (totem.currentName())
                        + "   /   " + juce::String (totem.currentNumber()).paddedLeft ('0', 2)
                        + " OF " + juce::String (totem.librarySize());
    if (isOn (pLock))
        status += "   /   HELD";
    g.setColour (lk.textDim.withAlpha (0.75f));
    g.drawText (status, r, juce::Justification::centredLeft, false);

    // right: tempo source, honest about whether a beat grid exists
    const bool synced = colours.beatValid();
    const juce::String tempo = synced ? juce::String (transport.bpm, 1) + " BPM" : juce::String ("FREE");
    auto right = r;
    const auto dot = right.removeFromRight (8.0f).withSizeKeepingCentre (5.0f, 5.0f);
    const float pulse = synced ? colours.pulse() : analyser.features().kick;
    g.setColour (look::accent().withAlpha (0.3f + 0.7f * juce::jlimit (0.0f, 1.0f, pulse * 2.0f)));
    g.fillEllipse (dot);
    right.removeFromRight (6.0f);
    g.setColour (lk.textDim.withAlpha (0.75f));
    g.drawText (tempo, right, juce::Justification::centredRight, false);
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
        g.drawText ("TOTEM 2", r, juce::Justification::centredLeft, false);
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
    g.drawText ("TOTEM 2", r, juce::Justification::centredLeft, false);

    // a short accent hairline under the wordmark
    g.setColour (look::accent().withAlpha (0.8f));
    g.fillRect (r.getX() + 1.0f, r.getCentreY() + h * 0.62f, 18.0f, 1.0f);
}
}

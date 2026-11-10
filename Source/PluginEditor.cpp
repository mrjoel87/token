#include "PluginEditor.h"
#include "Palette.h"
#include <cmath>

namespace token
{
namespace
{
    juce::Image makeBrushedTexture()
    {
        juce::Image img (juce::Image::ARGB, 512, 256, true, juce::SoftwareImageType());
        juce::Random r (42);
        {
            juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < img.getHeight(); ++y)
            {
                float v = r.nextFloat() - 0.5f;
                const float rowBias = (r.nextFloat() - 0.5f) * 0.6f;

                for (int x = 0; x < img.getWidth(); ++x)
                {
                    v = v * 0.985f + (r.nextFloat() - 0.5f) * 0.12f;
                    const float s = juce::jlimit (-1.0f, 1.0f, v * 2.0f + rowBias);
                    data.setPixelColour (x, y, s > 0.0f ? juce::Colours::white.withAlpha (s * 0.10f)
                                                         : juce::Colours::black.withAlpha (-s * 0.16f));
                }
            }
        }
        return juce::NativeImageType().convert (img);
    }

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

    juce::Image makeScanTexture()
    {
        juce::Image img (juce::Image::ARGB, 4, 3, true, juce::SoftwareImageType());
        {
            juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
            for (int x = 0; x < 4; ++x)
                data.setPixelColour (x, 0, juce::Colours::black.withAlpha (0.35f));
        }
        return juce::NativeImageType().convert (img);
    }

    void drawScrew (juce::Graphics& g, juce::Point<float> c, float r, float angle)
    {
        juce::ColourGradient grad (juce::Colour (0xff6a707a), c.x - r * 0.5f, c.y - r * 0.5f,
                                   juce::Colour (0xff15171a), c.x + r, c.y + r, true);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 0.8f);

        const auto d = juce::Point<float> (std::cos (angle), std::sin (angle)) * (r * 0.7f);
        g.setColour (juce::Colours::black.withAlpha (0.75f));
        g.drawLine ({ c - d, c + d }, 1.2f);
    }

    bool on (const std::atomic<float>* p) { return p != nullptr && p->load() > 0.5f; }
}

// =================================================================================================
TokenEditor::TokenEditor (TokenAudioProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      modeSelector    (*p.apvts.getParameter (ids::mode),      { "STRIP", "BOX" }),
      sceneSelector   (*p.apvts.getParameter (ids::scene),     { "AUTO", "MIN", "DREAM", "MYSTIC", "NORSE" }),
      displaySelector (*p.apvts.getParameter (ids::display),   { "TRIG", "SCROLL" }),
      stereoToggle    (*p.apvts.getParameter (ids::stereo),    "STEREO"),
      gainToggle      (*p.apvts.getParameter (ids::autoGain),  "AGC"),
      spectrumToggle  (*p.apvts.getParameter (ids::spectrum),  "SPEC"),
      cleanToggle     (*p.apvts.getParameter (ids::clean),     "CLEAN"),
      syncSelector    (*p.apvts.getParameter (ids::sync),      { "FREE", "AUTO", "1/4", "1/2", "1", "2", "4", "8", "16" }),
      speedSelector   (*p.apvts.getParameter (ids::speed),     { "SLOW", "NORM", "FAST", "AUTO" }),
      themeSelector   (*p.apvts.getParameter (ids::theme),     { "DARK", "LIGHT", "TEAL", "GOLD" }),
      allToggle       (*p.apvts.getParameter (ids::allImages), "ALL"),
      lostToggle      (*p.apvts.getParameter (ids::catLost),   "LOST"),
      occultToggle    (*p.apvts.getParameter (ids::catOccult), "OCCULT"),
      humanToggle     (*p.apvts.getParameter (ids::catHuman),  "HUMAN"),
      natureToggle    (*p.apvts.getParameter (ids::catNature), "NATURE"),
      roadsToggle     (*p.apvts.getParameter (ids::catRoads),  "ROADS"),
      rareToggle      (*p.apvts.getParameter (ids::rare),      "RARE APPARITIONS")
{
    pTheme = p.apvts.getRawParameterValue (ids::theme);
    applyCurrentTheme();

    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    for (auto* k : { &reactKnob, &chaosKnob, &glowKnob, &densityKnob, &mysteryKnob, &visibilityKnob })
    {
        k->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k->setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
        k->setMouseDragSensitivity (220);
        addAndMakeVisible (*k);
    }

    reactAttachment      = std::make_unique<SliderAttachment> (p.apvts, ids::react,      reactKnob);
    chaosAttachment      = std::make_unique<SliderAttachment> (p.apvts, ids::chaos,      chaosKnob);
    glowAttachment       = std::make_unique<SliderAttachment> (p.apvts, ids::glow,       glowKnob);
    densityAttachment    = std::make_unique<SliderAttachment> (p.apvts, ids::density,    densityKnob);
    mysteryAttachment    = std::make_unique<SliderAttachment> (p.apvts, ids::mystery,    mysteryKnob);
    visibilityAttachment = std::make_unique<SliderAttachment> (p.apvts, ids::visibility, visibilityKnob);

    reactKnob.setDoubleClickReturnValue      (true, defaults::react);
    chaosKnob.setDoubleClickReturnValue      (true, defaults::chaos);
    glowKnob.setDoubleClickReturnValue       (true, defaults::glow);
    densityKnob.setDoubleClickReturnValue    (true, defaults::density);
    mysteryKnob.setDoubleClickReturnValue    (true, defaults::mystery);
    visibilityKnob.setDoubleClickReturnValue (true, defaults::visibility);

    for (auto* c : std::initializer_list<juce::Component*> { &modeSelector, &sceneSelector, &displaySelector,
                                                             &stereoToggle, &gainToggle, &spectrumToggle, &cleanToggle,
                                                             &syncSelector, &speedSelector, &themeSelector,
                                                             &allToggle, &lostToggle, &occultToggle, &humanToggle,
                                                             &natureToggle, &roadsToggle, &rareToggle })
        addAndMakeVisible (c);

    pMode       = p.apvts.getRawParameterValue (ids::mode);
    pReact      = p.apvts.getRawParameterValue (ids::react);
    pChaos      = p.apvts.getRawParameterValue (ids::chaos);
    pGlow       = p.apvts.getRawParameterValue (ids::glow);
    pScene      = p.apvts.getRawParameterValue (ids::scene);
    pClean      = p.apvts.getRawParameterValue (ids::clean);
    pDisplay    = p.apvts.getRawParameterValue (ids::display);
    pStereo     = p.apvts.getRawParameterValue (ids::stereo);
    pAutoGain   = p.apvts.getRawParameterValue (ids::autoGain);
    pSpectrum   = p.apvts.getRawParameterValue (ids::spectrum);
    pAll        = p.apvts.getRawParameterValue (ids::allImages);
    pCategories = { p.apvts.getRawParameterValue (ids::catLost),   p.apvts.getRawParameterValue (ids::catOccult),
                    p.apvts.getRawParameterValue (ids::catHuman),  p.apvts.getRawParameterValue (ids::catNature),
                    p.apvts.getRawParameterValue (ids::catRoads) };
    pSync       = p.apvts.getRawParameterValue (ids::sync);
    pSpeed      = p.apvts.getRawParameterValue (ids::speed);
    pDensity    = p.apvts.getRawParameterValue (ids::density);
    pMystery    = p.apvts.getRawParameterValue (ids::mystery);
    pVisibility = p.apvts.getRawParameterValue (ids::visibility);
    pRare       = p.apvts.getRawParameterValue (ids::rare);

    pullLeft.resize ((size_t) ScopeFifo::capacity);
    pullRight.resize ((size_t) ScopeFifo::capacity);

    brushedTexture = makeBrushedTexture();
    grainTexture   = makeGrainTexture();
    scanTexture    = makeScanTexture();

    clean = pClean->load() > 0.5f;
    applyingMode = true;
    setResizable (true, true);
    applyMode (pMode->load() > 0.5f ? DisplayMode::Box : DisplayMode::Strip, true);

    startTime = lastTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
    vblank = std::make_unique<juce::VBlankAttachment> (this, std::function<void()> ([this] { frame(); }));
}

TokenEditor::~TokenEditor()
{
    vblank.reset();
    setLookAndFeel (nullptr);
}

void TokenEditor::applyCurrentTheme()
{
    // The palette is shared by every TOKEN window, so each one re-applies its own theme before use.
    pal::applyTheme (pTheme != nullptr ? juce::jlimit (0, 3, juce::roundToInt (pTheme->load())) : 0);
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
        setResizeLimits (560, 120, 3200, 560);
    else
        setResizeLimits (640, 520, 3200, 2200);

    const int w = mode == DisplayMode::Strip ? proc.stripW.load() : proc.boxW.load();
    const int h = mode == DisplayMode::Strip ? proc.stripH.load() : proc.boxH.load();
    setSize (w, h);

    applyingMode = false;
    updateVisibility();
    resized();
}

void TokenEditor::updateVisibility()
{
    const bool showPanel = ! clean;
    const bool showBox = ! clean && mode == DisplayMode::Box;

    for (auto* c : std::initializer_list<juce::Component*> { &reactKnob, &chaosKnob, &glowKnob,
                                                             &modeSelector, &sceneSelector, &displaySelector })
        c->setVisible (showPanel);

    for (auto* c : std::initializer_list<juce::Component*> { &stereoToggle, &gainToggle, &spectrumToggle,
                                                             &densityKnob, &mysteryKnob, &visibilityKnob,
                                                             &syncSelector, &speedSelector, &themeSelector,
                                                             &allToggle, &lostToggle, &occultToggle, &humanToggle,
                                                             &natureToggle, &roadsToggle, &rareToggle })
        c->setVisible (showBox);

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

    if (clean)                           layoutClean();
    else if (mode == DisplayMode::Strip) layoutStrip();
    else                                 layoutBox();
}

void TokenEditor::placeKnobs (juce::Rectangle<float> area, std::array<juce::Slider*, 3> knobs, std::array<const char*, 3> names,
                              float knobSize, float labelHeight)
{
    const float cellW = area.getWidth() / 3.0f;
    const float top = area.getCentreY() - (knobSize + labelHeight) * 0.5f;

    for (size_t i = 0; i < 3; ++i)
    {
        const float cx = area.getX() + cellW * ((float) i + 0.5f);
        knobs[i]->setBounds (juce::Rectangle<float> (cx - knobSize * 0.5f, top, knobSize, knobSize).toNearestInt());
        labels.push_back ({ names[i], juce::Rectangle<float> (cx - cellW * 0.5f, top + knobSize, cellW, labelHeight),
                            juce::jlimit (8.0f, 11.0f, labelHeight * 0.72f), juce::Justification::centred });
    }
}

void TokenEditor::layoutBox()
{
    labels.clear();
    const auto b = getLocalBounds().toFloat();
    auto area = b.reduced (18.0f);

    const float deckH = juce::jlimit (200.0f, 290.0f, b.getHeight() * 0.34f);
    auto deck = area.removeFromBottom (deckH);
    area.removeFromBottom (16.0f);
    screenRect = area;
    panelRect = deck;

    auto inner = deck.reduced (14.0f, 10.0f);
    auto row1 = inner.removeFromTop ((inner.getHeight() - 10.0f) * 0.5f);
    inner.removeFromTop (10.0f);
    auto row2 = inner;
    txRowRect = row2;

    const float leftW  = juce::jmin (250.0f, row1.getWidth() * 0.28f);
    const float rightW = juce::jmin (340.0f, row1.getWidth() * 0.42f);
    const float capW = 46.0f;
    const float labelH = 13.0f;

    auto placeColumn = [this, capW] (juce::Rectangle<float> col, std::initializer_list<std::pair<juce::Component*, const char*>> items)
    {
        const int n = (int) items.size();
        const float rowH = juce::jmin (19.0f, (col.getHeight() - 6.0f) / (float) n);
        const float gap = n > 1 ? (col.getHeight() - rowH * (float) n) / (float) (n - 1) : 0.0f;
        int i = 0;
        for (const auto& item : items)
        {
            auto r = juce::Rectangle<float> (col.getX(), col.getY() + (float) i * (rowH + gap), col.getWidth(), rowH);
            if (item.second != nullptr)
                labels.push_back ({ item.second, r.removeFromLeft (capW), 8.5f, juce::Justification::centredLeft });
            item.first->setBounds (r.toNearestInt());
            ++i;
        }
    };

    // ---- row 1: brand | REACT CHAOS GLOW | MODE SCENE SCOPE OPTIONS
    brandRect = row1.removeFromLeft (leftW);
    auto right1 = row1.removeFromRight (rightW);
    row1.reduce (8.0f, 0.0f);
    const float knob1 = juce::jmax (28.0f, juce::jmin (row1.getHeight() - labelH - 2.0f, row1.getWidth() / 3.0f - 12.0f));
    placeKnobs (row1, { &reactKnob, &chaosKnob, &glowKnob }, { "REACT", "CHAOS", "GLOW" }, knob1, labelH);

    placeColumn (right1.removeFromTop (right1.getHeight() * 0.76f),
                 { { &modeSelector, "MODE" }, { &sceneSelector, "SCENE" }, { &displaySelector, "SCOPE" } });
    {
        auto leds = right1.withTrimmedTop (2.0f);
        labels.push_back ({ "OPTION", leds.removeFromLeft (capW), 8.5f, juce::Justification::centredLeft });
        const float lw = leds.getWidth() / 4.0f;
        stereoToggle.setBounds   (leds.removeFromLeft (lw).toNearestInt());
        gainToggle.setBounds     (leds.removeFromLeft (lw).toNearestInt());
        spectrumToggle.setBounds (leds.removeFromLeft (lw).toNearestInt());
        cleanToggle.setBounds    (leds.toNearestInt());
    }

    // ---- row 2: TRANSMISSION categories | DENSITY MYSTERY VISIBLE | SYNC SPEED THEME RARE
    auto cats = row2.removeFromLeft (leftW);
    auto right2 = row2.removeFromRight (rightW);
    row2.reduce (8.0f, 0.0f);

    labels.push_back ({ "HIDDEN TRANSMISSION", cats.removeFromTop (16.0f), 9.0f, juce::Justification::centredLeft });
    {
        const float cellH = juce::jmin (20.0f, cats.getHeight() / 2.0f);
        auto r1 = cats.removeFromTop (cellH);
        auto r2 = cats.removeFromTop (cellH);
        const float cw = cats.getWidth() / 3.0f;
        allToggle.setBounds    (r1.removeFromLeft (cw).toNearestInt());
        lostToggle.setBounds   (r1.removeFromLeft (cw).toNearestInt());
        occultToggle.setBounds (r1.toNearestInt());
        humanToggle.setBounds  (r2.removeFromLeft (cw).toNearestInt());
        natureToggle.setBounds (r2.removeFromLeft (cw).toNearestInt());
        roadsToggle.setBounds  (r2.toNearestInt());
    }

    const float knob2 = juce::jmax (28.0f, juce::jmin (row2.getHeight() - labelH - 2.0f, row2.getWidth() / 3.0f - 12.0f));
    placeKnobs (row2, { &densityKnob, &mysteryKnob, &visibilityKnob }, { "DENSITY", "MYSTERY", "VISIBLE" }, knob2, labelH);

    placeColumn (right2, { { &syncSelector, "SYNC" }, { &speedSelector, "SPEED" }, { &themeSelector, "THEME" },
                           { &rareToggle, "EXTRA" } });
}

void TokenEditor::layoutStrip()
{
    labels.clear();
    const auto b = getLocalBounds().toFloat();
    auto area = b.reduced (14.0f);

    const float panelW = juce::jlimit (230.0f, 300.0f, b.getWidth() * 0.25f);
    auto panel = area.removeFromRight (panelW);
    area.removeFromRight (12.0f);
    screenRect = area;
    panelRect = panel;
    txRowRect = {};

    auto inner = panel.reduced (8.0f, 5.0f);
    brandRect = inner.removeFromTop (juce::jmin (24.0f, inner.getHeight() * 0.18f));

    const float rowH = juce::jlimit (12.0f, 17.0f, inner.getHeight() * 0.1f);
    auto row2 = inner.removeFromBottom (rowH);
    inner.removeFromBottom (4.0f);
    auto row1 = inner.removeFromBottom (rowH);
    inner.removeFromBottom (2.0f);

    sceneSelector.setBounds (row1.toNearestInt());
    modeSelector.setBounds (row2.removeFromLeft (row2.getWidth() * 0.38f).toNearestInt());
    row2.removeFromLeft (4.0f);
    displaySelector.setBounds (row2.removeFromLeft (row2.getWidth() * 0.6f).toNearestInt());
    row2.removeFromLeft (4.0f);
    cleanToggle.setBounds (row2.toNearestInt());

    const float labelH = 12.0f;
    const float knobSize = juce::jmax (24.0f, juce::jmin (inner.getHeight() - labelH, inner.getWidth() / 3.0f - 8.0f));
    placeKnobs (inner, { &reactKnob, &chaosKnob, &glowKnob }, { "REACT", "CHAOS", "GLOW" }, knobSize, labelH);
}

void TokenEditor::layoutClean()
{
    labels.clear();
    screenRect = getLocalBounds().toFloat().reduced (3.0f);
    panelRect = brandRect = txRowRect = {};
    cleanToggle.setBounds (getWidth() - 28, 8, 20, 20);
}

juce::Rectangle<float> TokenEditor::scopeArea() const
{
    const auto s = screenRect;
    return mode == DisplayMode::Strip ? s.reduced (s.getWidth() * 0.01f, s.getHeight() * 0.10f)
                                      : s.reduced (s.getWidth() * 0.02f, s.getHeight() * 0.12f);
}

juce::Rectangle<float> TokenEditor::spectrumArea() const
{
    const auto s = screenRect;
    return { s.getX() + s.getWidth() * 0.03f, s.getBottom() - s.getHeight() * 0.2f, s.getWidth() * 0.94f, s.getHeight() * 0.17f };
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
ScopeSettings TokenEditor::readScopeSettings() const
{
    ScopeSettings s;
    s.scrolling = on (pDisplay);
    s.stereo    = on (pStereo);
    s.autoGain  = on (pAutoGain);
    s.spectrum  = on (pSpectrum);
    s.glow      = pGlow->load();
    s.react     = pReact->load();
    s.strip     = mode == DisplayMode::Strip;
    return s;
}

tx::TxSettings TokenEditor::readTxSettings() const
{
    tx::TxSettings s;
    const bool all = on (pAll);
    for (size_t i = 0; i < s.categories.size(); ++i)
        s.categories[i] = all || on (pCategories[i]);

    s.sync       = (SyncMode)  juce::jlimit (0, 8, juce::roundToInt (pSync->load()));
    s.speed      = (SpeedMode) juce::jlimit (0, 3, juce::roundToInt (pSpeed->load()));
    s.density    = pDensity->load();
    s.mystery    = pMystery->load();
    s.visibility = pVisibility->load();
    s.rare       = on (pRare);
    return s;
}

tx::Transport TokenEditor::readTransport (double now) const
{
    tx::Transport t;
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

    applyCurrentTheme();

    // discrete parameters that change the layout (may come from the host / automation)
    const bool newClean = on (pClean);
    if (newClean != clean)
    {
        clean = newClean;
        updateVisibility();
        resized();
    }

    const auto newMode = on (pMode) ? DisplayMode::Box : DisplayMode::Strip;
    if (newMode != mode)
        applyMode (newMode, false);

    sceneMode = (SceneMode) juce::jlimit (0, 4, juce::roundToInt (pScene->load()));

    // ---- audio from the processor
    const int n = proc.scopeFifo.pull (pullLeft.data(), pullRight.data(), (int) pullLeft.size());
    if (n > 0)
        history.append (pullLeft.data(), pullRight.data(), n);

    const double sr = proc.currentSampleRate.load();
    analyser.process (history, n, sr, dt);
    const auto& f = analyser.features();

    // ---- scope + world
    scopeSettings = readScopeSettings();
    scope.update (history, f, scopeSettings, scopeArea(), sr, dt);

    ctx.screen = screenRect;
    ctx.px = juce::jlimit (2.0f, 5.0f, std::round (screenRect.getHeight() / 110.0f));
    ctx.time = (float) (now - startTime);
    ctx.dt = dt;
    ctx.react = pReact->load();
    ctx.chaos = pChaos->load();
    ctx.glow = pGlow->load();
    ctx.strip = mode == DisplayMode::Strip;
    ctx.audio = &f;
    ctx.scope = &scope;
    world.update (ctx, sceneMode);

    // ---- hidden transmission
    txCtx.screen = screenRect;
    txCtx.px = ctx.px;
    txCtx.time = ctx.time;
    txCtx.dt = dt;
    txCtx.audio = &f;
    txCtx.scope = &scope;
    txCtx.particles = &world.getParticles();
    txCtx.settings = readTxSettings();
    txCtx.transport = readTransport (now);
    transmission.update (txCtx);

    hasFrame = true;
    repaint();
}

// =================================================================================================
//  Painting
// =================================================================================================
void TokenEditor::paint (juce::Graphics& g)
{
    applyCurrentTheme();
    paintChassis (g);

    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (screenRect, clean ? 2.0f : 6.0f);
        g.reduceClipRegion (clip);

        paintScreen (g);
        paintScreenOverlay (g);
        if (! clean)
            paintHud (g);
    }

    if (! clean)
        paintPanel (g);
}

void TokenEditor::paintChassis (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    juce::ColourGradient metal (pal::chassisTop, 0.0f, 0.0f, pal::chassisBottom, 0.0f, b.getBottom(), false);
    metal.addColour (0.06, pal::chassisSheen);
    metal.addColour (0.55, pal::chassisMid);
    g.setGradientFill (metal);
    g.fillRect (b);

    g.setColour (juce::Colours::black);
    g.setOpacity (pal::lightScreen ? 0.35f : 0.6f);
    g.drawImage (brushedTexture, b);
    g.setOpacity (1.0f);

    if (clean)
    {
        g.setColour (pal::goldDark.withAlpha (0.8f));
        g.drawRoundedRectangle (screenRect.expanded (1.5f), 3.0f, 1.0f);
        return;
    }

    // edges and inlay
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawHorizontalLine (0, 0.0f, b.getRight());
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawHorizontalLine (getHeight() - 1, 0.0f, b.getRight());
    g.setColour (pal::goldDark.withAlpha (0.55f));
    g.drawRoundedRectangle (b.reduced (5.5f), 5.0f, 1.0f);

    // corner screws
    const float sr = 4.0f, inset = 11.0f;
    drawScrew (g, { inset, inset }, sr, 0.6f);
    drawScrew (g, { b.getRight() - inset, inset }, sr, 2.1f);
    drawScrew (g, { inset, b.getBottom() - inset }, sr, 1.2f);
    drawScrew (g, { b.getRight() - inset, b.getBottom() - inset }, sr, 0.2f);

    // screen bezel
    const auto bezel = screenRect.expanded (7.0f);
    juce::ColourGradient bz (pal::bezelTop, 0.0f, bezel.getY(), pal::bezelBottom, 0.0f, bezel.getBottom(), false);
    g.setGradientFill (bz);
    g.fillRoundedRectangle (bezel, 9.0f);
    g.setColour (pal::gold.withAlpha (0.55f));
    g.drawRoundedRectangle (bezel.reduced (0.5f), 9.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (pal::lightScreen ? 0.4f : 1.0f));
    g.drawRoundedRectangle (screenRect.expanded (1.5f), 7.0f, 2.0f);

    // engraved graduation along the bezel
    const bool below = mode == DisplayMode::Box;
    const float ty = below ? bezel.getBottom() + 3.0f : bezel.getY() - 3.0f;
    for (int i = 0; i <= 40; ++i)
    {
        const float x = screenRect.getX() + screenRect.getWidth() * (float) i / 40.0f;
        const float len = i % 5 == 0 ? 4.0f : 2.0f;
        g.setColour (pal::gold.withAlpha (i % 10 == 0 ? 0.5f : 0.22f));
        g.fillRect (x - 0.5f, below ? ty : ty - len, 1.0f, len);
    }
}

void TokenEditor::paintScreen (juce::Graphics& g)
{
    g.setColour (pal::black);
    g.fillRect (screenRect);

    if (! hasFrame)
        return;

    const auto& f = analyser.features();

    world.drawBackground (g, ctx);
    world.drawEvents (g, ctx, Layer::Back);
    transmission.draw (g, txCtx);

    if (mode == DisplayMode::Box && scopeSettings.spectrum)
        scope.drawSpectrum (g, f, spectrumArea(), scopeSettings.glow);

    world.drawMidEffects (g, ctx);
    world.drawEvents (g, ctx, Layer::Mid);
    scope.draw (g, f, scopeSettings);
    world.drawEvents (g, ctx, Layer::Front);
    world.drawParticles (g);
}

void TokenEditor::paintScreenOverlay (juce::Graphics& g)
{
    const auto s = screenRect;

    // film grain (re-anchored every frame so it shimmers) and faint scanlines
    g.setTiledImageFill (grainTexture, fxRandom.nextInt (256), fxRandom.nextInt (256), 0.05f);
    g.fillRect (s);
    g.setTiledImageFill (scanTexture, 0, (int) s.getY(), pal::lightScreen ? 0.25f : 0.6f);
    g.fillRect (s);

    // elliptical vignette
    const auto c = s.getCentre();
    juce::ColourGradient vig (juce::Colours::transparentBlack, c.x, c.y,
                              juce::Colours::black.withAlpha (pal::vignette), c.x + s.getWidth() * 0.62f, c.y, true);
    juce::FillType vigFill (vig);
    vigFill.transform = juce::AffineTransform::scale (1.0f, s.getHeight() / juce::jmax (1.0f, s.getWidth()) * 1.3f, c.x, c.y);
    g.setFillType (vigFill);
    g.fillRect (s);

    // glass: diagonal reflection, top highlight, inner shadow
    juce::ColourGradient glass (juce::Colours::white.withAlpha (0.06f), s.getX(), s.getY(),
                                juce::Colours::transparentWhite, s.getX() + s.getWidth() * 0.45f, s.getY() + s.getHeight() * 0.9f, false);
    g.setGradientFill (glass);
    g.fillRect (s);

    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.fillRect (s.getX() + 8.0f, s.getY() + 1.0f, s.getWidth() - 16.0f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (pal::lightScreen ? 0.25f : 0.55f));
    g.drawRoundedRectangle (s, clean ? 2.0f : 6.0f, 5.0f);
}

void TokenEditor::paintHud (juce::Graphics& g)
{
    const auto& f = analyser.features();
    auto hud = screenRect.reduced (12.0f, 9.0f).removeFromTop (11.0f);
    const char* names[] = { "DREAM", "PULSE", "CHAOS" };
    const juce::Colour cols[] = { pal::turquoise, pal::cyan, pal::crimson.interpolatedWith (pal::gold, 0.4f) };

    g.setFont (ui::font (8.5f, false, 0.2f));

    for (int i = 0; i < 3; ++i)
    {
        const float w = f.stateWeights[(size_t) i];
        auto cell = hud.removeFromLeft (54.0f);
        const auto led = cell.removeFromLeft (8.0f).withSizeKeepingCentre (4.0f, 4.0f);
        g.setColour (cols[i].withAlpha (0.15f + 0.85f * w));
        g.fillEllipse (led);
        if (w > 0.3f)
        {
            g.setColour (cols[i].withAlpha (0.2f * w));
            g.fillEllipse (led.expanded (3.0f));
        }
        g.setColour (pal::moon.withAlpha (0.18f + 0.5f * w));
        g.drawText (names[i], cell, juce::Justification::centredLeft, false);
    }

    static const char* sceneNames[] = { "AUTO", "MINIMAL", "DREAM", "MYSTIC", "NORSE" };
    static const char* themeNames[] = { "DREAM", "MYSTIC", "NORSE" };
    const auto& th = world.getTheme();
    const int dominant = th[1] > th[0] ? (th[2] > th[1] ? 2 : 1) : (th[2] > th[0] ? 2 : 0);
    const juce::String right = sceneMode == SceneMode::Auto
                                   ? juce::String ("AUTO  /  ") + themeNames[dominant]
                                   : juce::String (sceneNames[(int) sceneMode]);
    g.setColour (pal::gold.withAlpha (0.45f));
    g.drawText (right, hud, juce::Justification::centredRight, false);

    // transmission readout, bottom left
    const auto status = transmission.getStatus();
    if (status.isNotEmpty())
    {
        auto bottom = screenRect.reduced (12.0f, 9.0f).removeFromBottom (11.0f);
        const bool live = txCtx.transport.playing;
        g.setColour (pal::cyan.withAlpha (0.25f + 0.2f * std::abs (std::sin (ctx.time * 1.7f))));
        g.fillEllipse (bottom.removeFromLeft (8.0f).withSizeKeepingCentre (4.0f, 4.0f));
        g.setColour (pal::moon.withAlpha (0.4f));
        g.drawText (status + (live ? "   /   SYNC" : ""), bottom, juce::Justification::centredLeft, false);
    }
}

void TokenEditor::paintPanel (juce::Graphics& g)
{
    juce::ColourGradient pg (pal::panelTop, 0.0f, panelRect.getY(), pal::panelBottom, 0.0f, panelRect.getBottom(), false);
    g.setGradientFill (pg);
    g.fillRoundedRectangle (panelRect, 6.0f);
    g.setColour (juce::Colours::black.withAlpha (pal::lightScreen ? 0.3f : 0.7f));
    g.drawRoundedRectangle (panelRect.reduced (0.5f), 6.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawHorizontalLine ((int) panelRect.getBottom(), panelRect.getX() + 6.0f, panelRect.getRight() - 6.0f);
    g.setColour (pal::goldDark.withAlpha (0.5f));
    g.drawRoundedRectangle (panelRect.reduced (3.0f), 4.0f, 0.6f);

    if (! txRowRect.isEmpty())
    {
        const float y = txRowRect.getY() - 5.0f;
        g.setColour (juce::Colours::black.withAlpha (pal::lightScreen ? 0.2f : 0.6f));
        g.fillRect (panelRect.getX() + 12.0f, y, panelRect.getWidth() - 24.0f, 1.0f);
        g.setColour (pal::goldDark.withAlpha (0.45f));
        g.fillRect (panelRect.getX() + 12.0f, y + 1.0f, panelRect.getWidth() - 24.0f, 1.0f);
    }

    paintBrand (g);

    for (const auto& l : labels)
        ui::engrave (g, l.text, l.area, l.height, pal::gold.withAlpha (0.75f), l.justification);
}

void TokenEditor::paintBrand (juce::Graphics& g)
{
    auto r = brandRect;
    if (r.isEmpty())
        return;

    auto goldText = [&g] (const juce::String& text, juce::Rectangle<float> area, float height, juce::Justification just)
    {
        g.setFont (ui::font (height, true, 0.32f));
        g.setColour (juce::Colours::black.withAlpha (pal::lightScreen ? 0.2f : 0.65f));
        g.drawText (text, area.translated (0.0f, 1.5f), just, false);
        juce::ColourGradient grad (pal::goldBright, 0.0f, area.getCentreY() - height * 0.5f,
                                   pal::goldDark, 0.0f, area.getCentreY() + height * 0.5f, false);
        grad.addColour (0.55, pal::gold);
        g.setGradientFill (grad);
        g.drawText (text, area, just, false);
    };

    if (mode == DisplayMode::Box)
    {
        const float th = juce::jlimit (18.0f, 34.0f, r.getHeight() * 0.34f);
        ui::engrave (g, "SMSW", r.removeFromTop (th * 0.6f), th * 0.36f, pal::cyan.withAlpha (0.6f), juce::Justification::centredLeft, true);
        goldText ("TOKEN", r.removeFromTop (th * 1.2f), th, juce::Justification::centredLeft);
        ui::engrave (g, "THE LIVING OSCILLOSCOPE", r.removeFromTop (13.0f), 8.5f, pal::gold.withAlpha (0.55f), juce::Justification::centredLeft);
        ui::engrave (g, "MODEL T-1   /   V1.1", r.removeFromTop (13.0f), 7.5f, pal::gold.withAlpha (0.3f), juce::Justification::centredLeft);
    }
    else
    {
        const float th = juce::jlimit (12.0f, 20.0f, r.getHeight() * 0.8f);
        goldText ("TOKEN", r, th, juce::Justification::centredLeft);
        ui::engrave (g, "SMSW", r, th * 0.45f, pal::cyan.withAlpha (0.6f), juce::Justification::centredRight, true);
    }
}
}

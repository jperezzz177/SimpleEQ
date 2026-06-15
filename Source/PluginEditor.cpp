/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const auto backgroundTop = juce::Colour(34u, 35u, 34u);
const auto backgroundBottom = juce::Colour(11u, 12u, 13u);
const auto panelTop = juce::Colour(38u, 38u, 36u);
const auto panelBottom = juce::Colour(18u, 19u, 19u);
const auto analyzerPurple = juce::Colour(92u, 55u, 145u);
const auto knobRed = juce::Colour(196u, 34u, 31u);
const auto knobRedDark = juce::Colour(78u, 12u, 15u);
const auto amber = juce::Colour(255u, 151u, 24u);
const auto gold = juce::Colour(219u, 176u, 112u);
const auto green = juce::Colour(151u, 230u, 48u);

void drawBrushedMetal(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setGradientFill(juce::ColourGradient(backgroundTop, bounds.getCentreX(), bounds.getY(),
                                           backgroundBottom, bounds.getCentreX(), bounds.getBottom(),
                                           false));
    g.fillRoundedRectangle(bounds, 8.f);

    g.setColour(juce::Colours::white.withAlpha(0.025f));
    for (auto y = bounds.getY() + 2.f; y < bounds.getBottom(); y += 3.f)
        g.drawHorizontalLine(juce::roundToInt(y), bounds.getX() + 2.f, bounds.getRight() - 2.f);

    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.drawRoundedRectangle(bounds.reduced(1.f), 8.f, 1.5f);

    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawRoundedRectangle(bounds.reduced(3.f), 6.f, 1.f);
}

void drawInsetPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float corner = 8.f)
{
    g.setGradientFill(juce::ColourGradient(panelTop, bounds.getCentreX(), bounds.getY(),
                                           panelBottom, bounds.getCentreX(), bounds.getBottom(),
                                           false));
    g.fillRoundedRectangle(bounds, corner);

    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.drawRoundedRectangle(bounds, corner, 2.f);

    g.setColour(juce::Colours::white.withAlpha(0.11f));
    g.drawRoundedRectangle(bounds.reduced(2.f), corner - 2.f, 1.f);
}

void drawScrew(juce::Graphics& g, juce::Point<float> centre)
{
    auto r = juce::Rectangle<float>(centre.x - 7.f, centre.y - 7.f, 14.f, 14.f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(84u, 82u, 78u), r.getX(), r.getY(),
                                           juce::Colours::black, r.getRight(), r.getBottom(), false));
    g.fillEllipse(r);
    g.setColour(juce::Colours::black.withAlpha(0.85f));
    g.drawEllipse(r, 1.f);
    g.drawLine(r.getX() + 4.f, centre.y, r.getRight() - 4.f, centre.y, 1.4f);
}
}

void LookAndFeel::drawRotarySlider(juce::Graphics & g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider & slider)
{
    using namespace juce;
    
    auto bounds = Rectangle<float>(x, y, width, height).reduced(2.f);
    auto enabled = slider.isEnabled();

    auto centre = bounds.getCentre();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

    g.setColour(Colours::black.withAlpha(0.45f));
    g.fillEllipse(bounds.translated(3.f, 5.f));

    auto outer = bounds.reduced(radius * 0.03f);
    g.setGradientFill(ColourGradient(Colour(245u, 231u, 205u), outer.getX(), outer.getY(),
                                     Colour(35u, 32u, 31u), outer.getRight(), outer.getBottom(),
                                     false));
    g.fillEllipse(outer);

    g.setColour(Colours::black.withAlpha(0.85f));
    g.drawEllipse(outer, 2.f);

    auto ring = outer.reduced(radius * 0.13f);
    g.setGradientFill(ColourGradient(Colour(18u, 20u, 22u), ring.getX(), ring.getY(),
                                     Colour(1u, 1u, 2u), ring.getRight(), ring.getBottom(),
                                     false));
    g.fillEllipse(ring);

    g.setColour(enabled ? Colour(235u, 235u, 230u) : Colours::grey);
    g.drawEllipse(ring.reduced(radius * 0.06f), 2.2f);

    auto face = ring.reduced(radius * 0.18f);
    g.setGradientFill(ColourGradient(enabled ? Colour(238u, 62u, 54u) : Colour(55u, 55u, 58u), face.getCentreX(), face.getY(),
                                     enabled ? knobRedDark : Colour(24u, 24u, 25u), face.getCentreX(), face.getBottom(),
                                     false));
    g.fillEllipse(face);

    g.setColour(knobRed.withAlpha(enabled ? 0.45f : 0.12f));
    g.drawEllipse(face.reduced(radius * 0.05f), 2.f);

    g.setColour(Colours::white.withAlpha(0.18f));
    g.drawEllipse(face.reduced(1.f), 1.f);
    
    if( auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        Path p;
        
        Rectangle<float> r;
        r.setLeft(centre.getX() - 2.7f);
        r.setRight(centre.getX() + 2.7f);
        r.setTop(face.getY() + 5.f);
        r.setBottom(centre.getY() - rswl->getTextHeight() * 1.65f);
        
        p.addRoundedRectangle(r, 2.f);
        
        jassert(rotaryStartAngle < rotaryEndAngle);
        
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        
        p.applyTransform(AffineTransform().rotated(sliderAngRad, centre.getX(), centre.getY()));

        g.setColour(enabled ? Colours::white : Colours::grey);
        g.fillPath(p);

        g.setColour(enabled ? amber.withAlpha(0.3f) : Colours::transparentBlack);
        g.strokePath(p, PathStrokeType(3.2f, PathStrokeType::curved, PathStrokeType::rounded));
        
        auto text = rswl->getDisplayString();
        auto maxTextWidth = face.getWidth() * 0.8f;
        auto fontSize = juce::jlimit(11.f, 19.f, face.getWidth() * 0.18f);
        g.setFont(Font(fontSize, Font::bold));
        auto strWidth = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), text);
        while (strWidth > maxTextWidth && fontSize > 11.f)
        {
            fontSize -= 1.f;
            g.setFont(Font(fontSize, Font::bold));
            strWidth = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), text);
        }
        
        r.setSize(strWidth + 10.f, fontSize + 8.f);
        r.setCentre(centre);
        
        g.setColour(Colours::black.withAlpha(enabled ? 0.62f : 0.42f));
        g.fillRoundedRectangle(r, 3.f);
        
        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics &g,
                                   juce::ToggleButton &toggleButton,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown)
{
    using namespace juce;
    
    if( auto* pb = dynamic_cast<PowerButton*>(&toggleButton) )
    {
        Path powerButton;
        
        auto bounds = toggleButton.getLocalBounds();
        
        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 4;
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();
        
        g.setColour(Colours::black.withAlpha(0.5f));
        g.fillEllipse(r.translated(1.5f, 2.f));

        g.setGradientFill(ColourGradient(Colour(68u, 70u, 66u), r.getX(), r.getY(),
                                         Colours::black, r.getRight(), r.getBottom(), false));
        g.fillEllipse(r);

        auto inner = r.reduced(4.f);
        g.setColour(Colour(17u, 19u, 17u));
        g.fillEllipse(inner);

        float ang = 34.f;
        
        size -= 6;
        
        powerButton.addCentredArc(r.getCentreX(),
                                  r.getCentreY(),
                                  size * 0.5,
                                  size * 0.5,
                                  0.f,
                                  degreesToRadians(ang),
                                  degreesToRadians(360.f - ang),
                                  true);
        
        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());
        
        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);
        
        auto color = toggleButton.getToggleState() ? Colours::dimgrey : green;
        
        if (! toggleButton.getToggleState())
        {
            g.setColour(green.withAlpha(0.18f));
            g.fillEllipse(r.expanded(3.f));
        }

        g.setColour(color);
        g.strokePath(powerButton, pst);
        g.drawEllipse(inner, 1.5f);
    }
    else if( auto* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton) )
    {
        auto color = ! toggleButton.getToggleState() ? Colours::dimgrey : green;
        auto bounds = toggleButton.getLocalBounds();
        auto r = bounds.reduced(2).toFloat();

        g.setColour(Colours::black.withAlpha(0.65f));
        g.fillRoundedRectangle(r, 4.f);
        g.setColour(Colours::white.withAlpha(0.28f));
        g.drawRoundedRectangle(r, 4.f, 1.f);
        
        g.setColour(color);
        g.strokePath(analyzerButton->randomPath, PathStrokeType(1.f));
    }
}
//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    
    auto range = getRange();
    
    auto sliderBounds = getSliderBounds();
    
//    g.setColour(Colours::red);
//    g.drawRect(getLocalBounds());
//    g.setColour(Colours::yellow);
//    g.drawRect(sliderBounds);
    
    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng,
                                      endAng,
                                      *this);
    
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    const auto enabled = isEnabled();

    g.setColour(Colours::white.withAlpha(enabled ? 0.72f : 0.25f));
    for (int i = 0; i <= 28; ++i)
    {
        const auto prop = static_cast<float>(i) / 28.f;
        const auto ang = jmap(prop, 0.f, 1.f, startAng, endAng);
        const auto outer = center.getPointOnCircumference(radius + 9.f, ang);
        const auto inner = center.getPointOnCircumference(radius + (i % 4 == 0 ? 3.f : 6.f), ang);
        g.drawLine(Line<float>(inner, outer), i % 4 == 0 ? 1.3f : 0.9f);
    }
    
    g.setColour(enabled ? green : Colours::grey);
    g.setFont(Font(getTextHeight(), Font::bold));
    
    auto numChoices = labels.size();
    for( int i = 0; i < numChoices; ++i )
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        
        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
        
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 1.15f + 12.f, ang);
        
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(
            juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), str),
            getTextHeight()
        );
        r.setCentre(c);
        r.setY(r.getY() + 3);
        
        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
    
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    
    auto size = juce::jmin(bounds.getWidth() - 58, bounds.getHeight() - getTextHeight() * 3 - 20);
    size = juce::jmax(36, size);

    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), bounds.getY() + size / 2 + 15);
    
    return r;
    
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if( auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param) )
        return choiceParam->getCurrentChoiceName();
    
    juce::String str;
    bool addK = false;
    
    if( auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param) )
    {
        float val = getValue();
        
        if( val > 999.f )
        {
            val /= 1000.f; //1001 / 1000 = 1.001
            addK = true;
        }
        
        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse; //this shouldn't happen!
    }
    
    if( suffix.isNotEmpty() )
    {
        str << " ";
        if( addK )
            str << "k";
        
        str << suffix;
    }
    
    return str;
}
//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) :
audioProcessor(p),
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->addListener(this);
    }

    updateChain();
    
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::updateResponseCurve()
{
    using namespace juce;
    auto responseArea = getAnalysisArea();
    
    auto w = responseArea.getWidth();
    
    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();
    
    auto sampleRate = audioProcessor.getSampleRate();
    
    std::vector<double> mags;
    
    mags.resize(w);
    
    for( int i = 0; i < w; ++i )
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        
        if(! monoChain.isBypassed<ChainPositions::Peak>() )
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if( !monoChain.isBypassed<ChainPositions::LowCut>() )
        {
            if( !lowcut.isBypassed<0>() )
                mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if( !lowcut.isBypassed<1>() )
                mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if( !lowcut.isBypassed<2>() )
                mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if( !lowcut.isBypassed<3>() )
                mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        if( !monoChain.isBypassed<ChainPositions::HighCut>() )
        {
            if( !highcut.isBypassed<0>() )
                mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if( !highcut.isBypassed<1>() )
                mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if( !highcut.isBypassed<2>() )
                mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if( !highcut.isBypassed<3>() )
                mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
            
        mags[i] = Decibels::gainToDecibels(mag);
    }
    
    responseCurve.clear();
    
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -48.0, 24.0, outputMin, outputMax);
    };
    
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    
    for( size_t i = 1; i < mags.size(); ++i )
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    drawInsetPanel(g, getLocalBounds().toFloat().reduced(1.f), 8.f);

    auto renderArea = getRenderArea().toFloat();
    g.setColour(Colour(7u, 10u, 11u));
    g.fillRoundedRectangle(renderArea, 7.f);

    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.04f), renderArea.getX(), renderArea.getY(),
                                     Colours::white.withAlpha(0.f), renderArea.getX(), renderArea.getBottom(), false));
    g.fillRoundedRectangle(renderArea, 7.f);

    drawBackgroundGrid(g);
    
    auto responseArea = getAnalysisArea();
    
    if( shouldShowFFTAnalysis )
    {
        auto leftChannelFFTPath = leftPathProducer.getPath();
        leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        
        g.setColour(analyzerPurple.withAlpha(0.82f));
        g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));
        
        auto rightChannelFFTPath = rightPathProducer.getPath();
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        
        g.setColour(gold.withAlpha(0.78f));
        g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    }
    
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.4f, PathStrokeType::curved, PathStrokeType::rounded));
    
    Path border;
    
    border.setUsingNonZeroWinding(false);
    
    border.addRoundedRectangle(getRenderArea(), 4);
    border.addRectangle(getLocalBounds());
    
    g.setColour(Colour(8u, 9u, 10u));
    
    g.fillPath(border);
    
    drawTextLabels(g);
    
    g.setColour(gold.withAlpha(0.82f));
    g.drawRoundedRectangle(getRenderArea().toFloat(), 7.f, 1.f);
}

std::vector<float> ResponseCurveComponent::getFrequencies()
{
    return std::vector<float>
    {
        20, /*30, 40,*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000
    };
}

std::vector<float> ResponseCurveComponent::getGains()
{
    return std::vector<float>
    {
        -48, -36, -24, -12, 0, 12, 24
    };
}

std::vector<float> ResponseCurveComponent::getXs(const std::vector<float> &freqs, float left, float width)
{
    std::vector<float> xs;
    for( auto f : freqs )
    {
        auto normX = juce::mapFromLog10(f, 20.f, 20000.f);
        xs.push_back( left + width * normX );
    }
    
    return xs;
}

void ResponseCurveComponent::drawBackgroundGrid(juce::Graphics &g)
{
    using namespace juce;
    auto freqs = getFrequencies();
    
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    auto xs = getXs(freqs, left, width);
    
    g.setColour(Colours::white.withAlpha(0.12f));
    for( auto x : xs )
    {
        g.drawVerticalLine(x, top, bottom);
    }
    
    auto gain = getGains();
    
    for( auto gDb : gain )
    {
        auto y = jmap(gDb, -48.f, 24.f, float(bottom), float(top));
        
        g.setColour(gDb == 0.f ? green.withAlpha(0.76f) : Colours::white.withAlpha(0.10f));
        g.drawHorizontalLine(y, left, right);
    }
}

void ResponseCurveComponent::drawTextLabels(juce::Graphics &g)
{
    using namespace juce;
    g.setColour(Colours::lightgrey);
    const int fontHeight = 13;
    g.setFont(Font(fontHeight, Font::plain));
    
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    auto freqs = getFrequencies();
    auto xs = getXs(freqs, left, width);
    
    for( int i = 0; i < freqs.size(); ++i )
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if( f > 999.f )
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if( addK )
            str << "k";
        str << "Hz";
        
        auto textWidth = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), str);
        Rectangle<int> r;

        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(4);
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
    
    auto gain = getGains();

    for( auto gDb : gain )
    {
        auto y = jmap(gDb, -48.f, 24.f, float(bottom), float(top));
        
        String str;
        if( gDb > 0 )
            str << "+";
        str << gDb;
        
        auto textWidth = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);
        
        g.setColour(gDb == 0.f ? green : Colours::lightgrey );
        
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
        
        str.clear();
        if( gDb > 0 )
            str << "+";
        str << gDb;

        r.setX(1);
        textWidth = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), str);
        r.setSize(textWidth, fontHeight);
        g.setColour(gDb == 0.f ? green : Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
    }
}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    
    responseCurve.preallocateSpace(getWidth() * 3);
    updateResponseCurve();
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    while( leftChannelFifo->getNumCompleteBuffersAvailable() > 0 )
    {
        if( leftChannelFifo->getAudioBuffer(tempIncomingBuffer) )
        {
            auto size = tempIncomingBuffer.getNumSamples();

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                              monoBuffer.getReadPointer(0, size),
                                              monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                              tempIncomingBuffer.getReadPointer(0, 0),
                                              size);
            
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }
    
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / double(fftSize);

    while( leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0 )
    {
        std::vector<float> fftData;
        if( leftChannelFFTDataGenerator.getFFTData( fftData) )
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }
    
    while( pathProducer.getNumPathsAvailable() > 0 )
    {
        pathProducer.getPath( leftChannelFFTPath );
    }
}

void ResponseCurveComponent::timerCallback()
{
    if( shouldShowFFTAnalysis )
    {
        auto fftBounds = getAnalysisArea().toFloat();
        auto sampleRate = audioProcessor.getSampleRate();
        
        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);
    }

    if( parametersChanged.compareAndSetBool(false, true) )
    {
        updateChain();
        updateResponseCurve();
    }
    
    repaint();
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    
    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(),
                    lowCutCoefficients,
                    chainSettings.lowCutSlope);
    
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(),
                    highCutCoefficients,
                    chainSettings.highCutSlope);
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();
    
    bounds.removeFromTop(26);
    bounds.removeFromBottom(14);
    bounds.removeFromLeft(42);
    bounds.removeFromRight(42);
    
    return bounds;
}


juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}
//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
peakFreqSlider(*audioProcessor.apvts.getParameter("PeakFreq"), "Hz"),
peakGainSlider(*audioProcessor.apvts.getParameter("PeakGain"), "dB"),
peakQualitySlider(*audioProcessor.apvts.getParameter("PeakQuality"), ""),
lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCutFreq"), "Hz"),
highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCutFreq"), "Hz"),
lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCutSlope"), "dB/Oct"),
highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCutSlope"), "db/Oct"),

responseCurveComponent(audioProcessor),

peakFreqSliderAttachment(audioProcessor.apvts, "PeakFreq", peakFreqSlider),
peakGainSliderAttachment(audioProcessor.apvts, "PeakGain", peakGainSlider),
peakQualitySliderAttachment(audioProcessor.apvts, "PeakQuality", peakQualitySlider),
lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCutFreq", lowCutFreqSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "HighCutFreq", highCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCutSlope", lowCutSlopeSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCutSlope", highCutSlopeSlider),

lowcutBypassButtonAttachment(audioProcessor.apvts, "LowCutBypassed", lowcutBypassButton),
peakBypassButtonAttachment(audioProcessor.apvts, "PeakBypassed", peakBypassButton),
highcutBypassButtonAttachment(audioProcessor.apvts, "HighCutBypassed", highcutBypassButton),
analyzerEnabledButtonAttachment(audioProcessor.apvts, "AnalyzerEnabled", analyzerEnabledButton)
{
    peakFreqSlider.labels.add({0.f, "20Hz"});
    peakFreqSlider.labels.add({1.f, "20kHz"});
    
    peakGainSlider.labels.add({0.f, "-24dB"});
    peakGainSlider.labels.add({1.f, "+24dB"});
    
    peakQualitySlider.labels.add({0.f, "0.1"});
    peakQualitySlider.labels.add({1.f, "10.0"});
    
    lowCutFreqSlider.labels.add({0.f, "20Hz"});
    lowCutFreqSlider.labels.add({1.f, "20kHz"});
    
    highCutFreqSlider.labels.add({0.f, "20Hz"});
    highCutFreqSlider.labels.add({1.f, "20kHz"});
    
    lowCutSlopeSlider.labels.add({0.0f, "12"});
    lowCutSlopeSlider.labels.add({1.f, "48"});
    
    highCutSlopeSlider.labels.add({0.0f, "12"});
    highCutSlopeSlider.labels.add({1.f, "48"});
    
    for( auto* comp : getComps() )
    {
        addAndMakeVisible(comp);
    }
    
    peakBypassButton.setLookAndFeel(&lnf);
    highcutBypassButton.setLookAndFeel(&lnf);
    lowcutBypassButton.setLookAndFeel(&lnf);

    analyzerEnabledButton.setLookAndFeel(&lnf);
    
    auto safePtr = juce::Component::SafePointer<SimpleEQAudioProcessorEditor>(this);
    peakBypassButton.onClick = [safePtr]()
    {
        if( auto* comp = safePtr.getComponent() )
        {
            auto bypassed = comp->peakBypassButton.getToggleState();
            
            comp->peakFreqSlider.setEnabled( !bypassed );
            comp->peakGainSlider.setEnabled( !bypassed );
            comp->peakQualitySlider.setEnabled( !bypassed );
        }
    };
    

    lowcutBypassButton.onClick = [safePtr]()
    {
        if( auto* comp = safePtr.getComponent() )
        {
            auto bypassed = comp->lowcutBypassButton.getToggleState();
            
            comp->lowCutFreqSlider.setEnabled( !bypassed );
            comp->lowCutSlopeSlider.setEnabled( !bypassed );
        }
    };
    
    highcutBypassButton.onClick = [safePtr]()
    {
        if( auto* comp = safePtr.getComponent() )
        {
            auto bypassed = comp->highcutBypassButton.getToggleState();
            
            comp->highCutFreqSlider.setEnabled( !bypassed );
            comp->highCutSlopeSlider.setEnabled( !bypassed );
        }
    };

    analyzerEnabledButton.onClick = [safePtr]()
    {
        if( auto* comp = safePtr.getComponent() )
        {
            auto enabled = comp->analyzerEnabledButton.getToggleState();
            comp->responseCurveComponent.toggleAnalysisEnablement(enabled);
        }
    };
    
    setSize (920, 620);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    peakBypassButton.setLookAndFeel(nullptr);
    highcutBypassButton.setLookAndFeel(nullptr);
    lowcutBypassButton.setLookAndFeel(nullptr);

    analyzerEnabledButton.setLookAndFeel(nullptr);
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint(juce::Graphics &g)
{
    using namespace juce;
    
    auto bounds = getLocalBounds().toFloat();
    drawBrushedMetal(g, bounds.reduced(2.f));

    auto inner = getLocalBounds().reduced(12);
    auto header = inner.removeFromTop(62).toFloat();
    drawInsetPanel(g, header, 9.f);

    auto brandArea = header.toNearestInt().reduced(130, 4);
    g.setFont(Font(34.f, Font::bold));
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.96f), brandArea.getX(), brandArea.getY(),
                                     Colour(118u, 113u, 105u), brandArea.getX(), brandArea.getBottom(), false));
    g.drawFittedText("DaCartel", brandArea.removeFromTop(34), Justification::centred, 1);

    g.setFont(Font(17.f, Font::plain));
    g.setColour(gold.withAlpha(0.86f));
    g.drawFittedText("A U D I O   P L U G I N S", brandArea, Justification::centred, 1);

    auto analysisPanel = responseCurveComponent.getBounds().toFloat().expanded(8.f, 7.f);
    drawInsetPanel(g, analysisPanel, 10.f);

    auto controlPanel = inner.toFloat();
    controlPanel.removeFromTop(responseCurveComponent.getBottom() - inner.getY() + 18);
    drawInsetPanel(g, controlPanel, 10.f);

    drawScrew(g, controlPanel.getTopLeft() + Point<float>(18.f, 18.f));
    drawScrew(g, controlPanel.getTopRight() + Point<float>(-18.f, 18.f));
    drawScrew(g, controlPanel.getBottomLeft() + Point<float>(18.f, -18.f));
    drawScrew(g, controlPanel.getBottomRight() + Point<float>(-18.f, -18.f));

    g.setColour(Colours::black.withAlpha(0.65f));
    auto lowRight = lowCutFreqSlider.getBounds().getRight() + 28;
    auto highLeft = highCutFreqSlider.getBounds().getX() - 28;
    g.drawVerticalLine(lowRight, controlPanel.getY() + 26.f, controlPanel.getBottom() - 26.f);
    g.drawVerticalLine(highLeft, controlPanel.getY() + 26.f, controlPanel.getBottom() - 26.f);
    g.setColour(Colours::white.withAlpha(0.13f));
    g.drawVerticalLine(lowRight + 1, controlPanel.getY() + 26.f, controlPanel.getBottom() - 26.f);
    g.drawVerticalLine(highLeft + 1, controlPanel.getY() + 26.f, controlPanel.getBottom() - 26.f);

    auto drawCaption = [&g](const Slider& slider, const String& text)
    {
        auto r = slider.getBounds();
        r.setY(r.getBottom() - 19);
        r.setHeight(18);
        g.setFont(Font(15.f, Font::plain));
        g.setColour(Colours::lightgrey.withAlpha(0.86f));
        g.drawFittedText(text, r, Justification::centred, 1);
    };

    drawCaption(lowCutFreqSlider, "LOW CUT FREQUENCY");
    drawCaption(lowCutSlopeSlider, "LOW CUT SLOPE");
    drawCaption(peakFreqSlider, "PEAK FREQUENCY");
    drawCaption(peakGainSlider, "PEAK GAIN");
    drawCaption(peakQualitySlider, "PEAK Q");
    drawCaption(highCutFreqSlider, "HIGH CUT FREQUENCY");
    drawCaption(highCutSlopeSlider, "HIGH CUT SLOPE");
}

void SimpleEQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20, 16);
    auto header = bounds.removeFromTop(54);

    auto analyzerButtonArea = header.removeFromLeft(92).reduced(10, 8);
    analyzerEnabledButton.setBounds(analyzerButtonArea);

    bounds.removeFromTop(16);
    responseCurveComponent.setBounds(bounds.removeFromTop(180).reduced(46, 18));

    bounds.removeFromTop(22);
    auto controls = bounds.reduced(8, 8);

    auto lowCutArea = controls.removeFromLeft(controls.getWidth() * 0.32f).reduced(18, 0);
    auto highCutArea = controls.removeFromRight(controls.getWidth() * 0.47f).reduced(18, 0);
    auto peakArea = controls.reduced(34, 0);

    auto buttonSize = 38;
    lowcutBypassButton.setBounds(lowCutArea.removeFromTop(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize));
    highcutBypassButton.setBounds(highCutArea.removeFromTop(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize));
    peakBypassButton.setBounds(peakArea.removeFromTop(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize));

    lowCutArea.removeFromTop(4);
    highCutArea.removeFromTop(4);
    peakArea.removeFromTop(4);

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5f).reduced(8, 0));
    lowCutSlopeSlider.setBounds(lowCutArea.reduced(8, 0));

    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5f).reduced(8, 0));
    highCutSlopeSlider.setBounds(highCutArea.reduced(8, 0));

    peakFreqSlider.setBounds(peakArea.removeFromTop(peakArea.getHeight() * 0.34f).reduced(18, 0));
    peakGainSlider.setBounds(peakArea.removeFromTop(peakArea.getHeight() * 0.5f).reduced(24, 0));
    peakQualitySlider.setBounds(peakArea.reduced(34, 0));
}

std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent,
        
        &lowcutBypassButton,
        &peakBypassButton,
        &highcutBypassButton,
        &analyzerEnabledButton
    };
}

//  ==============================================================================
//  ==============================================================================
//  ==============================================================================
///*
//  ==============================================================================
//
//    This file contains the basic framework code for a JUCE plugin editor.
//
//  ==============================================================================
//*/
//
//#include "PluginProcessor.h"
//#include "PluginEditor.h"
//
//void LookAndFeel::drawRotarySlider(juce::Graphics & g,
//                                   int x,
//                                   int y,
//                                   int width,
//                                   int height,
//                                   float sliderPosProportional,
//                                   float rotaryStartAngle,
//                                   float rotaryEndAngle,
//                                   juce::Slider & slider)
//{
//    using namespace juce;
//    
//    auto bounds = Rectangle<float>(x, y, width, height);
//    
//    g.setColour(Colour(97u, 18u, 167u));
//    g.fillEllipse(bounds);
//    
//    g.setColour(Colour(255u, 154, 1u));
//    g.drawEllipse(bounds, 1.f);
//    
//    if(auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
//    {
//        auto center = bounds.getCentre();
//        Path p;
//        
//        Rectangle<float> r;
//        r.setLeft(center.getX() - 2);
//        r.setRight(center.getX() + 2);
//        r.setTop(bounds.getY());
//        r.setBottom(center.getY() - rswl->getTextHeight()*1.5);
//        
//        p.addRoundedRectangle(r, 2.f);
//        
//        jassert(rotaryStartAngle < rotaryEndAngle);
//        
//        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
//        
//        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
//        
//        g.fillPath(p);
//        
//        g.setFont(rswl->getTextHeight());
//        auto text= rswl->getDisplayString();
//        auto strWidth = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), text);
//        
//        r.setSize(strWidth+4, rswl->getTextHeight()+2);
//        r.setCentre(bounds.getCentre());
//        
//        g.setColour(Colours::black);
//        g.fillRect(r);
//        
//        g.setColour(Colours::white);
//        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
//
//    }
//    
//}
////==============================================================================
//void RotarySliderWithLabels::paint(juce::Graphics &g)
//{
//    using namespace juce;
//    
//    auto startAng = degreesToRadians(180.f + 45.f);
//    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
//    
//    auto range = getRange();
//    
//    auto sliderBounds = getSliderBounds();
//    
////    g.setColour(Colours::red);
////    g.drawRect(getLocalBounds());
////    g.setColour(Colours::yellow);
////    g.drawRect(sliderBounds);
//    
//    getLookAndFeel().drawRotarySlider(g,
//                                      sliderBounds.getX(),
//                                      sliderBounds.getY(),
//                                      sliderBounds.getWidth(),
//                                      sliderBounds.getHeight(),
//                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
//                                      startAng,
//                                      endAng,
//                                      *this);
//    
//    auto center = sliderBounds.toFloat().getCentre();
//    auto radius = sliderBounds.getWidth() * 0.5f;
//    
//    g.setColour(Colour(0u, 172u, 1u));
//    g.setFont(getTextHeight());
//    
//    auto numChoices = labels.size();
//    for( int i = 0; i < numChoices; ++i)
//    {
//        auto pos = labels[i].pos;
//        jassert(0.f <= pos);
//        jassert(pos <= 1.f);
//        
//        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
//        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
//        
//        Rectangle<float> r;
//        auto str = labels[i].label;
//        r.setSize(juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), str), static_cast<float>(getTextHeight()));
//        r.setCentre(c);
//        r.setY(r.getY()+ getTextHeight());
//        
//        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
//    }
//}
//
//juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
//{
//    
//    auto bounds = getLocalBounds();
//    
//    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
//    
//    size -= getTextHeight() * 2;
//    juce::Rectangle<int> r;
//    r.setSize(size, size);
//    r.setCentre(bounds.getCentreX(), 0);
//    r.setY(2);
//    
//    return r;
//}
//
//juce::String RotarySliderWithLabels::getDisplayString() const
//{
//    if(auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
//        return choiceParam->getCurrentChoiceName();
//    
//    juce::String str;
//    bool addK = false;
//    
//    if( auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
//    {
//        float val = getValue();
//        
//        if(val > 999.f)
//        {
//            val /= 1000.f;
//            addK = true;
//            
//        }
//        str = juce::String(val, (addK ? 2 : 0));
//    }
//    else
//    {
//        jassertfalse;
//    }
//    
//    if( suffix.isNotEmpty())
//    {
//        str << " ";
//        if (addK)
//            str << "k";
//        str << suffix;
//    }
//    
//    return str;
//}
////==============================================================================
//ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) :
//audioProcessor(p),
//leftChannelFifo(&audioProcessor.leftChannelFifo)
//{
//    const auto& params = audioProcessor.getParameters();
//    for (auto param : params)
//    {
//        param->addListener(this);
//    }
//    
//    leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
//    monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
//    
//    updateChain();
//    startTimer(60);
//}
//
//ResponseCurveComponent::~ResponseCurveComponent()
//{
//    const auto& params = audioProcessor.getParameters();
//    for (auto param : params)
//    {
//        param->removeListener(this);
//    }
//}
//
//void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
//{
//    parametersChanged.set(true);
//}
//
//void ResponseCurveComponent::timerCallback()
//{
//    juce::AudioBuffer<float> tempIncomingBuffer;
//    
//    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
//    {
//        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
//        {
//            auto size = tempIncomingBuffer.getNumSamples();
//            
//            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
//                                              monoBuffer.getReadPointer(0, size),
//                                              monoBuffer.getNumSamples() - size);
//            
//            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
//                                                                         tempIncomingBuffer.getReadPointer(0, 0),
//                                                                         size);
//            
//            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
//        }
//    }
//    
//    const auto fftBounds = getAnalysisArea().toFloat();
//    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
//    
//    const auto binWidth = audioProcessor.getSampleRate() / (double)fftSize;
//    
//    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
//    {
//        std::vector<float> fftData;
//        
//        if (leftChannelFFTDataGenerator.getFFTData(fftData))
//        {
//            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
//        }
//    }
//    
//    while (pathProducer.getNumPathsAvailable())
//    {
//        pathProducer.getPath(leftChannelFFTPath);
//    }
//    
//    if (parametersChanged.compareAndSetBool(false, true))
//    {
//        //update the monochain and signala repaint
//        updateChain();
////        repaint();
//    }
//    
//    repaint();
//}
//
//void ResponseCurveComponent::updateChain()
//{
//    auto chainSettings = getChainSettings(audioProcessor.apvts);
//    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
//    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
//    
//    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
//    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
//    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
//    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
//}
//
//void ResponseCurveComponent::paint (juce::Graphics& g)
//{
//    using namespace juce;
//    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    g.fillAll (Colours::black);
//    
//    g.drawImage(background, getLocalBounds().toFloat());
//    
//    auto responseArea = getAnalysisArea();
//    
//    auto w = responseArea.getWidth();
//    
//    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
//    auto& peak = monoChain.get<ChainPositions::Peak>();
//    auto& highcut = monoChain.get<ChainPositions::HighCut>();
//    
//    auto sampleRate = audioProcessor.getSampleRate();
//    
//    std::vector<double> mags;
//    
//    mags.resize(w);
//    
//    for (int i = 0; i < w; ++i) {
//        double mag = 1.f;
//        auto freq= mapToLog10(double(i) / double(w), 20.0, 20000.0);
//        
//        if(! monoChain.isBypassed<ChainPositions::Peak>())
//            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
//        
//        if(!lowcut.isBypassed<0>())
//            mag*=lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
//        if(!lowcut.isBypassed<1>())
//            mag*=lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
//        if(!lowcut.isBypassed<2>())
//            mag*=lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
//        if(!lowcut.isBypassed<3>())
//            mag*=lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
//        
//        if(!highcut.isBypassed<0>())
//            mag*=highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
//        if(!highcut.isBypassed<1>())
//            mag*=highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
//        if(!highcut.isBypassed<2>())
//            mag*=highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
//        if(!highcut.isBypassed<3>())
//            mag*=highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
//        
//        mags[i] = Decibels::gainToDecibels(mag);
//    }
//    
//    Path responseCurve;
//    
//    const double outputMin = responseArea.getBottom();
//    const double outputMax = responseArea.getY();
//    auto map = [outputMin, outputMax](double input)
//    {
//        return jmap(input, -24.0, 24.0, outputMin, outputMax);
//    };
//    
//    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
//    
//    for (size_t i = 1; i < mags.size(); ++i)
//    {
//        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
//    }
//    
//    g.setColour(Colours::blue);
//    g.strokePath(leftChannelFFTPath, PathStrokeType(1));
//    
//    g.setColour(Colours::orange);
//    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
//    
//    g.setColour(Colours::white);
//    g.strokePath(responseCurve, PathStrokeType(2.f));
//    
//}
//
//void ResponseCurveComponent::resized()
//{
//    using namespace juce;
//    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
//    
//    Graphics g(background);
//    
//    Array<float> freqs
//    {
//        20,/*30,40,*/50,100,
//        200,/*300,400,*/500,1000,
//        2000,/*3000,4000,*/5000,10000,
//        20000
//    };
//    
//    auto renderArea = getAnalysisArea();
//    auto left = renderArea.getX();
//    auto right = renderArea.getRight();
//    auto top = renderArea.getY();
//    auto bottom = renderArea.getBottom();
//    auto width = renderArea.getWidth();
//    
//    Array<float> xs;
//    for (auto f : freqs)
//    {
//        auto normX = mapFromLog10(f, 20.f, 20000.f);
//        xs.add(left + width * normX);
//    }
//    
//    g.setColour(Colours::dimgrey);
//
//    for( auto x : xs)
//    {
//
//        g.drawVerticalLine(x, top, bottom);
//    }
//    
//    Array<float> gain
//    {
//        -24, -12, 0, 12, 24
//    };
//    
//    for( auto gDb : gain)
//    {
//        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
//        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u): Colours::darkgrey);
//        g.drawHorizontalLine(y, left, right);
//    }
//    
//    g.setColour(Colours::lightgrey);
//    const int fontHeight = 10;
//    g.setFont(fontHeight);
//    
//    for(int i =0; i < freqs.size(); ++i)
//    {
//        auto f = freqs[i];
//        auto x = xs[i];
//        
//        bool addK = false;
//        String str;
//        if ( f > 999.f)
//        {
//            addK = true;
//            f /= 1000.f;
//            
//        }
//        
//        str << f;
//        if(addK)
//            str << "k";
//        str << "Hz";
//        
//        auto textWidth = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), str);
//        
//        Rectangle<int> r;
//        r.setSize(textWidth, fontHeight);
//        r.setCentre(x, 0);
//        r.setY(1);
//        
//        g.drawFittedText(str, r, juce::Justification::centred, 1);
//    }
//    
//    for( auto gDb : gain)
//    {
//        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
//        
//        String str;
//        if( gDb > 0 )
//            str << "+";
//        str << gDb;
//        
//        auto textWidth = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), str);
//        
//        Rectangle<int> r;
//        r.setSize(textWidth, fontHeight);
//        r.setX(getWidth() - textWidth);
//        r.setCentre(r.getCentreX(), y);
//        
//        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);
//        
//        g.drawFittedText(str, r, juce::Justification::centred, 1);
//        
//        str.clear();
//        str << (gDb - 24.f);
//        
//        r.setX(1);
//        textWidth = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), str);
//        r.setSize(textWidth, fontHeight);
//        g.setColour(Colours::lightgrey);
//        g.drawFittedText(str, r, juce::Justification::centred, 1);
//    }
//}
//
//juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
//{
//    auto bounds = getLocalBounds();
//    
//
//    bounds.removeFromTop(12);
//    bounds.removeFromBottom(2);
//    bounds.removeFromLeft(20);
//    bounds.removeFromRight(20);
//    
//    return bounds;
//}
//
//juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
//{
//    auto bounds = getRenderArea();
//    bounds.removeFromTop(4);
//    bounds.removeFromBottom(4);
//    return bounds;
//}
////==============================================================================
//SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
//    : AudioProcessorEditor (&p), audioProcessor (p),
//
//peakFreqSlider(*audioProcessor.apvts.getParameter("PeakFreq"), "Hz"),
//peakGainSlider(*audioProcessor.apvts.getParameter("PeakGain"), "dB"),
//peakQualitySlider(*audioProcessor.apvts.getParameter("PeakQuality"), ""),
//lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCutFreq"), "Hz"),
//highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCutFreq"), "Hz"),
//lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCutSlope"), "dB/Oct"),
//highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCutSlope"), "dB/Oct"),
//
//responseCurveComponent(audioProcessor),
//peakFreqSliderAttachment(audioProcessor.apvts, "PeakFreq", peakFreqSlider),
//peakGainSliderAttachment(audioProcessor.apvts, "PeakGain", peakGainSlider),
//peakQualitySliderAttachment(audioProcessor.apvts, "PeakQuality", peakQualitySlider),
//lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCutFreq", lowCutFreqSlider),
//highCutFreqSliderAttachment(audioProcessor.apvts, "HighCutFreq", highCutFreqSlider),
//lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCutSlope", lowCutSlopeSlider),
//highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCutSlope", highCutSlopeSlider)
//{
//    // Make sure that before the constructor has finished, you've set the
//    // editor's size to whatever you need it to be.
//    
//    peakFreqSlider.labels.add({0.f, "20Hz"});
//    peakFreqSlider.labels.add({1.f, "20kHz"});
//    
//    peakGainSlider.labels.add({0.f, "-24dB"});
//    peakGainSlider.labels.add({1.f, "+24dB"});
//    
//    peakQualitySlider.labels.add({0.f, "0.1"});
//    peakQualitySlider.labels.add({1.f, "10.0"});
//    
//    lowCutFreqSlider.labels.add({0.f, "20Hz"});
//    lowCutFreqSlider.labels.add({1.f, "20kHz"});
//    
//    highCutFreqSlider.labels.add({0.f, "20Hz"});
//    highCutFreqSlider.labels.add({1.f, "20kHz"});
//    
//    lowCutSlopeSlider.labels.add({0.f, "12"});
//    lowCutSlopeSlider.labels.add({1.f, "48"});
//    
//    highCutSlopeSlider.labels.add({0.f, "12"});
//    highCutSlopeSlider.labels.add({1.f, "48"});
//    
//    
//    for (auto* comp : getComps() )
//    {
//        addAndMakeVisible(comp);
//    }
//    
//    setSize (600, 480);
//}
//
//SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
//{
//   
//}
//
////==============================================================================
//void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
//{
//    using namespace juce;
//    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    g.fillAll (Colours::black);
//    
//}
//
//void SimpleEQAudioProcessorEditor::resized()
//{
//    // This is generally where you'll want to lay out the positions of any
//    // subcomponents in your editor..
//    
//    auto bounds = getLocalBounds();
//    float hRatio = 25.f / 100.f; //JUCE_LIVE_CONSTANT(33) / 100.f;
//    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);
//    
//    responseCurveComponent.setBounds(responseArea);
//    
//    bounds.removeFromTop(5);
//    
//    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
//    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
//    
//    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
//    lowCutSlopeSlider.setBounds(lowCutArea);
//    
//    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() *0.5));
//    highCutSlopeSlider.setBounds(highCutArea);
//    
//    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
//    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
//    peakQualitySlider.setBounds(bounds);
//}
//
//std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps()
//{
//    return
//    {
//        &peakFreqSlider,
//        &peakGainSlider,
//        &peakQualitySlider,
//        &lowCutFreqSlider,
//        &highCutFreqSlider,
//        &lowCutSlopeSlider,
//        &highCutSlopeSlider,
//        &responseCurveComponent
//    };
//}

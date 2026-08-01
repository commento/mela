#include "WaveformEditor.h"

#include <cmath>

void WaveformEditor::setClip(std::shared_ptr<const LoopEngine::Clip> newClip)
{
    clip = std::move(newClip);
    resetTrim();
}

void WaveformEditor::setPlayhead(double newPosition)
{
    playhead = juce::jlimit(0.0, 1.0, newPosition);
    repaint();
}

void WaveformEditor::resetTrim()
{
    trimStart = 0.0;
    trimEnd = 1.0;
    playhead = 0.0;
    viewStart = 0.0;
    viewEnd = 1.0;
    repaint();
}

void WaveformEditor::setEnvelope(double attack, double decay, float sustain,
                                 double release, double playbackRate,
                                 bool cycleEnabled)
{
    envelopeAttack = juce::jmax(0.0, attack);
    envelopeDecay = juce::jmax(0.0, decay);
    envelopeSustain = juce::jlimit(0.0f, 1.0f, sustain);
    envelopeRelease = juce::jmax(0.0, release);
    currentPlaybackRate = juce::jmax(0.01, playbackRate);
    envelopeCycleEnabled = cycleEnabled;
    repaint();
}

void WaveformEditor::paint(juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto plot = plotBounds();
    graphics.setColour(juce::Colour(0xff171b21));
    graphics.fillRoundedRectangle(bounds, 12.0f);

    if (clip == nullptr || clip->waveformMinimum.empty())
    {
        graphics.setColour(juce::Colours::white.withAlpha(0.55f));
        graphics.setFont(22.0f);
        graphics.drawText("Carica un file audio per visualizzare la forma d'onda",
                          getLocalBounds(), juce::Justification::centred);
        return;
    }

    const auto centreY = plot.getCentreY();
    const auto amplitude = plot.getHeight() * 0.42f;
    const auto points = static_cast<int>(clip->waveformMinimum.size());
    graphics.setColour(juce::Colour(0xff5ee6a8));

    for (int x = static_cast<int>(plot.getX()); x < static_cast<int>(plot.getRight()); ++x)
    {
        const auto sourcePosition = xToNormalised(static_cast<float>(x));
        const auto point = juce::jlimit(0, points - 1,
            static_cast<int>(sourcePosition * static_cast<double>(points)));
        const auto minimum = clip->waveformMinimum[static_cast<size_t>(point)];
        const auto maximum = clip->waveformMaximum[static_cast<size_t>(point)];
        graphics.drawVerticalLine(x, centreY - maximum * amplitude, centreY - minimum * amplitude);
    }

    const auto startX = normalisedToX(trimStart);
    const auto endX = normalisedToX(trimEnd);
    const auto clippedStartX = juce::jlimit(plot.getX(), plot.getRight(), startX);
    const auto clippedEndX = juce::jlimit(plot.getX(), plot.getRight(), endX);

    graphics.setColour(juce::Colour(0xcc080a0d));
    graphics.fillRect(juce::Rectangle<float>::leftTopRightBottom(
        plot.getX(), plot.getY(), clippedStartX, plot.getBottom()));
    graphics.fillRect(juce::Rectangle<float>::leftTopRightBottom(
        clippedEndX, plot.getY(), plot.getRight(), plot.getBottom()));

    if (envelopeCycleEnabled && trimEnd > trimStart)
    {
        const auto outputDuration = (trimEnd - trimStart) * durationSeconds()
                                  / currentPlaybackRate;
        auto attack = envelopeAttack;
        auto decay = envelopeDecay;
        auto release = envelopeRelease;
        const auto shapedDuration = attack + decay + release;

        if (shapedDuration > outputDuration && shapedDuration > 0.0)
        {
            const auto scale = outputDuration / shapedDuration;
            attack *= scale;
            decay *= scale;
            release *= scale;
        }

        const auto xAtTime = [this, outputDuration](double time)
        {
            const auto phase = outputDuration > 0.0 ? time / outputDuration : 0.0;
            return normalisedToX(trimStart + phase * (trimEnd - trimStart));
        };
        const auto envelopeTop = plot.getY() + 20.0f;
        const auto envelopeBottom = plot.getBottom() - 20.0f;
        const auto yAtLevel = [envelopeTop, envelopeBottom](float level)
        {
            return envelopeBottom - level * (envelopeBottom - envelopeTop);
        };

        juce::Path envelopePath;
        envelopePath.startNewSubPath(xAtTime(0.0), yAtLevel(0.0f));
        envelopePath.lineTo(xAtTime(attack), yAtLevel(1.0f));
        envelopePath.lineTo(xAtTime(attack + decay), yAtLevel(envelopeSustain));
        envelopePath.lineTo(xAtTime(juce::jmax(attack + decay,
                                               outputDuration - release)),
                            yAtLevel(envelopeSustain));
        envelopePath.lineTo(xAtTime(outputDuration), yAtLevel(0.0f));

        auto envelopeFill = envelopePath;
        envelopeFill.lineTo(xAtTime(outputDuration), envelopeBottom);
        envelopeFill.lineTo(xAtTime(0.0), envelopeBottom);
        envelopeFill.closeSubPath();

        graphics.saveState();
        graphics.reduceClipRegion(plot.toNearestInt());
        graphics.setColour(juce::Colour(0x225ea8ff));
        graphics.fillPath(envelopeFill);
        graphics.setColour(juce::Colour(0xff70a7ff));
        graphics.strokePath(envelopePath, juce::PathStrokeType(3.0f));
        graphics.restoreState();
    }

    constexpr auto handleWidth = 6.0f;
    graphics.setColour(juce::Colour(0xffffc857));
    if (trimStart >= viewStart && trimStart <= viewEnd)
        graphics.fillRect(startX, plot.getY(), handleWidth, plot.getHeight());
    if (trimEnd >= viewStart && trimEnd <= viewEnd)
        graphics.fillRect(endX, plot.getY(), handleWidth, plot.getHeight());

    if (playhead >= viewStart && playhead <= viewEnd
        && playhead >= trimStart && playhead <= trimEnd)
    {
        graphics.setColour(juce::Colours::white.withAlpha(0.9f));
        graphics.fillRect(normalisedToX(playhead), plot.getY(), 2.0f, plot.getHeight());
    }

    const auto duration = durationSeconds();
    const auto timeLabel = juce::String(trimStart * duration, 2) + " s  -  "
                         + juce::String(trimEnd * duration, 2) + " s";
    auto labelArea = getLocalBounds().removeFromBottom(34);
    graphics.setColour(juce::Colour(0xdd111419));
    graphics.fillRoundedRectangle(labelArea.toFloat().reduced(8.0f, 3.0f), 6.0f);
    graphics.setColour(juce::Colours::white);
    graphics.setFont(17.0f);
    graphics.drawText(timeLabel, labelArea, juce::Justification::centred);

    const auto zoom = 1.0 / (viewEnd - viewStart);
    graphics.setColour(juce::Colours::white.withAlpha(0.7f));
    graphics.setFont(14.0f);
    graphics.drawText("PINCH PER ZOOM  -  " + juce::String(zoom, 1) + "x",
                      getLocalBounds().reduced(16).removeFromTop(24),
                      juce::Justification::centredRight);
}

void WaveformEditor::mouseDown(const juce::MouseEvent& event)
{
    if (clip == nullptr)
        return;

    if (event.source.isTouch())
    {
        updateTouch(event, true);
        int activeTouches = 0;
        for (const auto& touch : touches)
            activeTouches += touch.active ? 1 : 0;

        if (activeTouches >= 2)
        {
            beginTouchPinch();
            draggedHandle = Handle::none;
            return;
        }
    }

    const auto startX = normalisedToX(trimStart);
    const auto endX = normalisedToX(trimEnd);
    const auto distanceFromStart = std::abs(event.position.x - startX);
    const auto distanceFromEnd = std::abs(event.position.x - endX);
    constexpr auto touchTarget = 34.0f;

    if (distanceFromStart <= touchTarget || distanceFromEnd <= touchTarget)
    {
        draggedHandle = distanceFromStart <= distanceFromEnd ? Handle::start : Handle::end;
        moveHandle(event.position.x);
    }
    else
    {
        draggedHandle = Handle::pan;
        dragOriginX = event.position.x;
        dragOriginViewStart = viewStart;
    }
}

void WaveformEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (event.source.isTouch())
    {
        updateTouch(event, true);
        int activeTouches = 0;
        for (const auto& touch : touches)
            activeTouches += touch.active ? 1 : 0;

        if (activeTouches >= 2)
        {
            if (! touchPinching)
                beginTouchPinch();
            updateTouchPinch();
            return;
        }
    }

    if (draggedHandle == Handle::pan)
        panView(event.position.x);
    else
        moveHandle(event.position.x);
}

void WaveformEditor::mouseUp(const juce::MouseEvent& event)
{
    if (event.source.isTouch())
    {
        updateTouch(event, false);
        int activeTouches = 0;
        for (const auto& touch : touches)
            activeTouches += touch.active ? 1 : 0;
        if (activeTouches < 2)
            touchPinching = false;
    }

    draggedHandle = Handle::none;
}

void WaveformEditor::mouseDoubleClick(const juce::MouseEvent&)
{
    viewStart = 0.0;
    viewEnd = 1.0;
    repaint();
}

void WaveformEditor::mouseMagnify(const juce::MouseEvent& event, float scaleFactor)
{
    zoomAround(event.position.x, static_cast<double>(scaleFactor));
}

void WaveformEditor::mouseWheelMove(const juce::MouseEvent& event,
                                    const juce::MouseWheelDetails& wheel)
{
    const auto scale = std::exp(static_cast<double>(wheel.deltaY) * 0.8);
    zoomAround(event.position.x, scale);
}

void WaveformEditor::moveHandle(float x)
{
    if ((draggedHandle != Handle::start && draggedHandle != Handle::end)
        || plotBounds().getWidth() <= 0.0f)
        return;

    constexpr auto minimumSelection = 0.005;
    const auto position = xToNormalised(x);

    if (draggedHandle == Handle::start)
        trimStart = juce::jmin(position, trimEnd - minimumSelection);
    else
        trimEnd = juce::jmax(position, trimStart + minimumSelection);

    if (onTrimChanged)
        onTrimChanged(trimStart, trimEnd);
    repaint();
}

void WaveformEditor::panView(float x)
{
    const auto span = viewEnd - viewStart;
    if (span >= 1.0)
        return;

    const auto delta = -static_cast<double>(x - dragOriginX)
                     / static_cast<double>(plotBounds().getWidth()) * span;
    viewStart = juce::jlimit(0.0, 1.0 - span, dragOriginViewStart + delta);
    viewEnd = viewStart + span;
    repaint();
}

void WaveformEditor::zoomAround(float x, double scaleFactor)
{
    if (clip == nullptr || ! std::isfinite(scaleFactor) || scaleFactor <= 0.0)
        return;

    const auto oldSpan = viewEnd - viewStart;
    const auto newSpan = juce::jlimit(0.01, 1.0, oldSpan / scaleFactor);
    const auto anchor = xToNormalised(x);
    const auto relativeX = juce::jlimit(0.0, 1.0,
        static_cast<double>(x - plotBounds().getX()) / plotBounds().getWidth());
    viewStart = juce::jlimit(0.0, 1.0 - newSpan, anchor - relativeX * newSpan);
    viewEnd = viewStart + newSpan;
    repaint();
}

void WaveformEditor::updateTouch(const juce::MouseEvent& event, bool isActive)
{
    const auto index = event.source.getIndex();
    if (! juce::isPositiveAndBelow(index, static_cast<int>(touches.size())))
        return;

    auto& touch = touches[static_cast<size_t>(index)];
    touch.active = isActive;
    touch.position = event.position;
}

void WaveformEditor::beginTouchPinch()
{
    const TouchState* first = nullptr;
    const TouchState* second = nullptr;
    for (const auto& touch : touches)
    {
        if (! touch.active)
            continue;
        if (first == nullptr)
            first = &touch;
        else
        {
            second = &touch;
            break;
        }
    }

    if (first == nullptr || second == nullptr)
        return;

    pinchInitialDistance = first->position.getDistanceFrom(second->position);
    pinchInitialSpan = viewEnd - viewStart;
    pinchAnchor = xToNormalised((first->position.x + second->position.x) * 0.5f);
    touchPinching = pinchInitialDistance > 1.0f;
}

void WaveformEditor::updateTouchPinch()
{
    const TouchState* first = nullptr;
    const TouchState* second = nullptr;
    for (const auto& touch : touches)
    {
        if (! touch.active)
            continue;
        if (first == nullptr)
            first = &touch;
        else
        {
            second = &touch;
            break;
        }
    }

    if (! touchPinching || first == nullptr || second == nullptr)
        return;

    const auto distance = first->position.getDistanceFrom(second->position);
    const auto newSpan = juce::jlimit(0.01, 1.0,
        pinchInitialSpan / juce::jmax(0.01, static_cast<double>(distance / pinchInitialDistance)));
    const auto midpointX = (first->position.x + second->position.x) * 0.5f;
    const auto relativeX = juce::jlimit(0.0, 1.0,
        static_cast<double>(midpointX - plotBounds().getX()) / plotBounds().getWidth());
    viewStart = juce::jlimit(0.0, 1.0 - newSpan, pinchAnchor - relativeX * newSpan);
    viewEnd = viewStart + newSpan;
    repaint();
}

juce::Rectangle<float> WaveformEditor::plotBounds() const
{
    return getLocalBounds().toFloat().reduced(10.0f, 0.0f);
}

float WaveformEditor::normalisedToX(double position) const
{
    const auto plot = plotBounds();
    return plot.getX() + static_cast<float>((position - viewStart) / (viewEnd - viewStart))
                       * plot.getWidth();
}

double WaveformEditor::xToNormalised(float x) const
{
    const auto plot = plotBounds();
    const auto visiblePosition = juce::jlimit(0.0, 1.0,
        static_cast<double>(x - plot.getX()) / plot.getWidth());
    return viewStart + visiblePosition * (viewEnd - viewStart);
}

double WaveformEditor::durationSeconds() const
{
    if (clip == nullptr || clip->sourceSampleRate <= 0.0)
        return 0.0;

    return static_cast<double>(clip->audio.getNumSamples()) / clip->sourceSampleRate;
}

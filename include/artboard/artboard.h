/*
 *  Arstro Artboard — aggregate header. Include this to use the platform-free core.
 *  Adapters (e.g. src/adapter/web/Canvas2DTarget.h) are included by the platform
 *  build, not here.
 */
#pragma once
#include "../../src/core/Geometry.h"
#include "../../src/core/Color.h"
#include "../../src/anim/Easing.h"
#include "../../src/anim/Animation.h"
#include "../../src/render/RenderTarget.h"
#include "../../src/render/RecordingTarget.h"
#include "../../src/input/Input.h"
#include "../../src/input/GestureRecognizer.h"
#include "../../src/input/InputRouter.h"
#include "../../src/scene/Drawable.h"
#include "../../src/scene/Shapes.h"
#include "../../src/scene/Artboard.h"
#include "../../src/ui/Property.h"
#include "../../src/ui/InputController.h"
#include "../../src/ui/Segment.h"
#include "../../src/ui/Theme.h"
#include "../../src/ui/Controls.h"

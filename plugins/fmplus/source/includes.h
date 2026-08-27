#pragma once
#include <plugmachine/plugmachine.h>
#include <tuning-library/tuning-library.h>
#include <juce_dsp/juce_dsp.h>
#include "BinaryData.h"
using namespace juce;

#define PC 11
#define MC 8
#define TABC MC+3
#define VC 24
#define FXC 8

#define MAXGLIDE 2
#define MINARP .01f
#define MAXARP 1
#define MINVIB .05f
#define MAXVIB 4
#define MAXVIBATT 2

#define MINA 0
#define MAXA 8
#define MIND .005f
#define MAXD 8
#define MINR .005f
#define MAXR 8
#define MINLFO .01f
#define MAXLFO 24
#define MAXLFOATT 8

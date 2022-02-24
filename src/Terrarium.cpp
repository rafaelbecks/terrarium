#include "daisysp.h"
#include "daisy_patch.h"
#include <string>
#include <math.h>

#include "lib/theory.h"

using namespace daisy;
using namespace daisysp;
using namespace ev_theory;

DaisyPatch patch;

uint8_t currentModule = 1;
std::string modules[3]    = {"turing", "scales", "passerine"};

void UpdateControls();
void SplashScreen();
void ModuleSelector();
void drawSine();

//turing
void UpdateTuringControls();
void UpdateTuringOled();
void UpdateTuringOutputs();
//scales
void UpdateScalesOled();
void UpdateScalesControls();
//passerine
void UpdatePasserineOled();
void UpdatePasserineControls();

void writeString(int x, int y, std::string s, FontDef size = Font_6x8)
{
    patch.display.SetCursor(x, y);
    char* cstr = &s[0];
    patch.display.WriteString(cstr, size, true);
}

void writeModuleTitle(std::string title) {
    patch.display.Fill(false);
    writeString(0,0, title);

    for(int i = 0; i < 128; i++)
    {
        patch.display.DrawPixel(i, 12, true);
    }
}

int main(void)
{
    patch.Init();
    patch.StartAdc();
    SplashScreen();
    while(1)
    {
        UpdateControls();
        ModuleSelector();
    }
}

void UpdateControls()
{
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();

    if(patch.encoder.Pressed())
    {
        currentModule = currentModule + patch.encoder.Increment();
        if(currentModule > 3)
        {
            currentModule = 1;
        }
        if(currentModule == 0)
        {
            currentModule = 3;
        }
    }
}

void ModuleSelector()
{
    UpdateTuringOutputs();
    switch(currentModule)
    {
        case 1:
            UpdateTuringControls();
            UpdateTuringOled();
            break;
        case 2:
            UpdateScalesOled();
            UpdateScalesControls();
            break;
        case 3:
            UpdatePasserineOled();
            UpdatePasserineControls();
            break;
        default: break;
    }
}

void SplashScreen()
{
    patch.display.Fill(false);
    // patch.display.DrawCircle(10,10,5, true);
    // patch.display.DrawCircle(118,54,5, true);
    // patch.display.DrawArc(20, 20, 10, 15, 15, true);
    // patch.display.DrawLine(84, 54, 100, 17, true);
    // patch.display.DrawLine(87, 47, 114, 29, true);
    // patch.display.DrawLine(87, 47, 82, 17, true);
    // patch.display.DrawLine(95, 25, 94, 16, true);
    // patch.display.DrawLine(96, 27, 106, 21, true);
    // patch.display.DrawLine(64, 8, 64, 55, true);
    drawSine();
    writeString(31, 27, "terrarium", Font_7x10);
    patch.display.Update();
    patch.DelayMs(2000);
}


void drawSine(){
    u_int8_t hCenter = 32;
    u_int8_t Radius  = 30;
    patch.display.Fill(false);
    patch.display.DrawRect(0, 0, 120, 63, true);

    for(int i = 0; i < 120; i++)
    {

        float Angle = i * 3;
        int   a     = (hCenter
                 + (sin(Angle * (PI_F / 180))
                    * Radius));

        patch.display.DrawPixel(i, a, true);
    }
}

/**
 * Turing machine by luukschipperheyn  https://github.com/luukschipperheyn/TuringMachine
 */

int       menuPos          = 0;
bool      inSubMenu        = false;
const int cursorAfterTitle = 15;

uint16_t values[]     = {0b1, 0b1};
uint8_t  amplitudes[] = {255, 255};
uint8_t  lengths[]    = {8, 8};
uint8_t  turingNote;
uint8_t  turing[] = {50,50};
uint8_t selectedRootNotes[2] = {0, 0};
uint8_t selectedModes[2] = {0, 0};
//passerine transpose
int8_t transform = 1;
int8_t interval = 1;
int8_t generate = 0;

DaisyPatch::Ctrl   turingControls[] = {DaisyPatch::CTRL_1, DaisyPatch::CTRL_3};
DaisyPatch::Ctrl   lengthControls[] = {DaisyPatch::CTRL_2, DaisyPatch::CTRL_4};
DacHandle::Channel cvChannels[]
    = {DacHandle::Channel::ONE, DacHandle::Channel::TWO};

uint16_t mapValue(int index)
{
    uint16_t value     = values[index];
    uint8_t  length    = lengths[index];
    uint8_t  amplitude = amplitudes[index];

    uint8_t  quantizedNotes[] = {1, 1};
    uint16_t voltageValues[] = {1,1};
    uint8_t semitonesToSum[6] = {0, 0, 7, 0, 7, 0};
    uint8_t  currentOctaves[] = {1, 1};
    uint8_t  currentNotes[]   = {1, 1};
    uint8_t  noteWithOctaves[] = {1, 1};
    int octavesToSum[6] =  {-1, 0, 0, 1, 1, 2};

    uint16_t maxValue = 0;
    for(int i = 0; i < length; i++)
    {
        maxValue = (maxValue << 1) | 1;
    }

    turingNote = round(((value & maxValue) / (maxValue / 255.f))
                       * (amplitude / 255.f));

    //octave
    currentOctaves[index] = ceil(turingNote / 51) + 1;

    currentOctaves[index] =  currentOctaves[index] + octavesToSum[transform]; //transpose

    //semitone
    currentNotes[index] = ceil(turingNote / 21.25);

    noteWithOctaves[index] = currentNotes[index] + (12 * currentOctaves[index]);
    quantizedNotes[index] = quantize(noteWithOctaves[index], modes[selectedModes[0]], selectedRootNotes[index]);

    quantizedNotes[index] = quantizedNotes[index] + (semitonesToSum[transform]); //transpose

    voltageValues[index] = semitoneToDac(quantizedNotes[index]);

    return prepareDacValForOutput(voltageValues[index]);
}

void UpdateTuringControls()
{
    inSubMenu = patch.encoder.RisingEdge() ? !inSubMenu : inSubMenu;
    if(!inSubMenu)
    {
        menuPos += patch.encoder.Increment();
        menuPos = (menuPos % 2 + 2) % 2;
    }
    else
    {
        amplitudes[menuPos] += (patch.encoder.Increment() * 17);
        amplitudes[menuPos] = amplitudes[menuPos] < 0 ? 0 : amplitudes[menuPos];
        amplitudes[menuPos]
            = amplitudes[menuPos] > 255 ? 255 : amplitudes[menuPos];
    }

    for(int i = 0; i < 2; i++)
    {
        uint8_t length = 1 + patch.GetKnobValue(lengthControls[i]) * 16;
        lengths[i]     = length;

        if(patch.gate_input[i].Trig())
        {
            uint16_t value       = values[i];
            turing[i] = patch.GetKnobValue(turingControls[i]) * 100;
            uint16_t lastBit     = value & (0b1 << (length - 1));
            uint16_t newBit      = lastBit >> (length - 1);
            int randomValue = rand() % 50;
            if(turing[i] < 98)
            {
                if(turing[i] < 2)
                {
                    newBit = newBit ^ 1;
                }
                else if(randomValue > abs(turing[i] - 50))
                {
                    newBit = rand() % 100 >= 50 ? 1 : 0;
                }
            }
            values[i] = value << 1 | newBit;
        }
    }
}

void UpdateTuringOled()
{

    writeModuleTitle("turing machine");

    char str[16];

    for(int i = 0; i < 2; i++)
    {
        for(int j = 0; j < lengths[i]; j++)
        {
            sprintf(str, "%u", values[i] >> j & 1);
            patch.display.SetCursor(120 - j * 8, cursorAfterTitle + (i * 25));
            patch.display.WriteString(str, Font_6x8, true);
        }

        sprintf(str, "amp: %d", amplitudes[i]);
        patch.display.SetCursor(80, cursorAfterTitle + (i * 25) + 12);
        patch.display.WriteString(str, Font_6x8, true);

        if(turing[i] < 48){
             writeString(0, i == 0 ? 27 : 52, "loop inv", Font_6x8);
        } else if(turing[i] > 98){
             writeString(0, i == 0 ? 27 : 52, "looping", Font_6x8);
        } else {
             writeString(0, i == 0 ? 27 : 52, "random", Font_6x8);
        }
    }


    //cursor
    writeString(70, menuPos == 0 ? 27 : 52, inSubMenu ? "x" : "o", Font_6x8);

    patch.display.Update();
}

void UpdateTuringOutputs()
{
    for(int i = 0; i < 2; i++)
    {
        patch.seed.dac.WriteValue(cvChannels[i], mapValue(i));
    }
}


/** scale selector */

void UpdateScalesControls() {

    DaisyPatch::Ctrl rootNoteControls[] = {DaisyPatch::CTRL_1, DaisyPatch::CTRL_3};
    DaisyPatch::Ctrl modeControls[] = {DaisyPatch::CTRL_2, DaisyPatch::CTRL_4};

    if(!patch.encoder.Pressed()){
        for(int i = 0; i < 2; i++){
            selectedRootNotes[i] = patch.GetKnobValue(rootNoteControls[i]) * 12;
            selectedModes[i] = patch.GetKnobValue(modeControls[i]) * 7;
        }
    }
}

void UpdateScalesOled() {

    patch.display.Fill(false);
    writeModuleTitle("scales");

    if(!patch.encoder.Pressed()){
        patch.display.DrawCircle(118, 4, 2,true);
    }

    writeString(26, 25, allNotes[selectedRootNotes[0]], Font_11x18);
    writeString(20, 44, modes[selectedModes[0]], Font_6x8);
    writeString(86, 25, allNotes[selectedRootNotes[1]], Font_11x18);
    writeString(79, 44, modes[selectedModes[1]], Font_6x8);
    patch.display.Update();
}

// passerine

std::string transformValues[6] = {"16", "8", "6", "4", "2.5", "2"};

void UpdatePasserineControls() {
    if(!patch.encoder.Pressed()){
        transform = patch.GetKnobValue(DaisyPatch::CTRL_1) * 6;
        interval = patch.GetKnobValue(DaisyPatch::CTRL_2) * 12;
        generate = patch.GetKnobValue(DaisyPatch::CTRL_3) * 2;
    }
}

void UpdatePasserineOled() {

    patch.display.Fill(false);
    writeModuleTitle("passerine");

    if(!patch.encoder.Pressed()){
        patch.display.DrawCircle(118, 4, 2,true);
    }

    writeString(6, 19, "transform", Font_6x8);
    writeString(6, 35, "interval", Font_6x8);
    writeString(6, 49, "generate", Font_6x8);
    writeString(104, 19, transformValues[transform], Font_6x8);
    writeString(104, 35, std::to_string(interval + 1), Font_6x8);
    writeString(104, 49, generate == 1 ? "on" : "off", Font_6x8);

    patch.display.Update();
}

#include "haltech_screen_entity.h"

// HaltechValueDisplay
HaltechValueDisplay::HaltechValueDisplay(HaltechDisplayType_e type) {
    this->type = type;
}

float HaltechValueDisplay::getValue() {
    return value;
}

std::string HaltechValueDisplay::getValueString(uint8_t decimalPlaces) {
    return std::string(value, decimalPlaces);
}

bool HaltechValueDisplay::isValueNew() {
    return valueNew;
}

void HaltechValueDisplay::clearValueNew() {
    valueNew = false;
}

bool HaltechValueDisplay::isButton() {
    return false;
}

void HaltechValueDisplay::updateValue(float value) {
    this->value = value;
    valueNew = true;
}

// HaltechButton
HaltechButton::HaltechButton() {
    for (int i = 0; i < 3; i++) {
        buttonStatus[i] = BUTTON_STATUS_OFF;
    }
}

bool HaltechButton::isButtonStatusOn(buttonStatusColor_e color) {
    return buttonStatus[color] == BUTTON_STATUS_ON;
}

bool HaltechButton::isButton() {
    return true;
}
#pragma once

void setupMQTT();
void handleMQTT();
void publishStatus(bool force, bool lightsOn, int brightness, bool motion);

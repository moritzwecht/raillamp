#pragma once

enum class ScheduleState {
  Unknown,
  Allowed,
  Blocked
};

void setupSchedule();
void handleSchedule();
ScheduleState getScheduleState();

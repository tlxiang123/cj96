#ifndef LOGIC_DISPLAYPOWERMANAGER_H_
#define LOGIC_DISPLAYPOWERMANAGER_H_

namespace DisplayPowerManager {

void syncFromContext();
void setTimeoutSeconds(int seconds);
int getTimeoutSeconds();
void setConfiguredTimeoutSeconds(int seconds);
int getConfiguredTimeoutSeconds();
void setTimeoutEnabled(bool enabled);
bool isTimeoutEnabled();

bool onOneSecondTimer();
bool handleTouchEvent();

bool captureScreenshotNow(const char* pageTag);

}  // namespace DisplayPowerManager

#endif /* LOGIC_DISPLAYPOWERMANAGER_H_ */

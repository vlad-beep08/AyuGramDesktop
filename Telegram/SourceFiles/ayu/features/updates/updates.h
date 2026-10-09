#pragma once

namespace Ui {
class Show;
} // namespace Ui

namespace AyuFeatures::Updates {

void Start();
void CheckNow(std::shared_ptr<Ui::Show> show);

[[nodiscard]] int CurrentBuild();

} // namespace AyuFeatures::Updates

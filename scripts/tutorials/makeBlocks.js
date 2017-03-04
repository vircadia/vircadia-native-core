// Toy
const MAX_RGB_COMPONENT_VALUE = 256 / 2; // Limit the values to half the maximum.
const MIN_COLOR_VALUE = 127;
function newColor() {
  color = {
    red: randomPastelRGBComponent(),
    green: randomPastelRGBComponent(),
    blue: randomPastelRGBComponent()
  };
  return color;
  }
// Helper functions.
function randomPastelRGBComponent() {
  return Math.floor(Math.random() * MAX_RGB_COMPONENT_VALUE) + MIN_COLOR_VALUE;
}

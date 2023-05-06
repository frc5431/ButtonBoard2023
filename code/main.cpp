#include <tuple>
#include <cstring>
#include <chrono>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "mcp3008.h"

using namespace std::chrono_literals;

// 1 MHz
#define SPI_SPEED 1000 * 1000
#define SPI_INSTANCE spi0
#define SPI_SCK_PIN 18
#define SPI_CSN_PIN 21
#define SPI_TX_PIN 19
#define SPI_RX_PIN 20
#define LED_PIN 25
#define LED_TIME 500ms

enum gamepad_joysticks
{
  left_x,
  left_y,
  left_z,
  right_x,
  right_y,
  right_z,
};

// Mappings for GPIO to HID
// Ideally these map 1:1 in their IDs but if a hotfix needs to be applied, it can be changed.
// For a layout of all pins, go to https://pico.pinout.xyz/
static constexpr std::pair<uint32_t, hid_gamepad_button_bm_t> DIGITAL_MAPPINGS[] = {
    {0, GAMEPAD_BUTTON_0},
    {1, GAMEPAD_BUTTON_1},
    {2, GAMEPAD_BUTTON_2},
    {3, GAMEPAD_BUTTON_3},
    {4, GAMEPAD_BUTTON_4},
    {5, GAMEPAD_BUTTON_5},
    {6, GAMEPAD_BUTTON_6},
    {7, GAMEPAD_BUTTON_7},
    //
    {10, GAMEPAD_BUTTON_9},
    {11, GAMEPAD_BUTTON_10},
    {12, GAMEPAD_BUTTON_11},
    //
    {13, GAMEPAD_BUTTON_13},
    {14, GAMEPAD_BUTTON_14},
    {15, GAMEPAD_BUTTON_15},

    // Out of order buttons
    {16, GAMEPAD_BUTTON_12},
    {17, GAMEPAD_BUTTON_8},
};

static constexpr std::pair<mcp3008::channels, gamepad_joysticks> ANALOG_MAPPINGS[] = {
    {mcp3008::CH0, left_x},
    {mcp3008::CH1, left_y},
    {mcp3008::CH2, left_z},
    {mcp3008::CH3, right_x},
    {mcp3008::CH4, right_y},
    {mcp3008::CH5, right_z},
};

static void hid_task();
static void hid_task_button(hid_gamepad_report_t &report);
static void hid_task_analog(hid_gamepad_report_t &report);

int main()
{
  board_init();
  tusb_init();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  for (auto [pin, _] : DIGITAL_MAPPINGS)
  {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
  }

  // Enable SPI 0 at 1 MHz and connect to GPIOs
  spi_init(SPI_INSTANCE, SPI_SPEED);
  gpio_set_function(SPI_CSN_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI_RX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI_TX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);

  auto update_time = std::chrono::high_resolution_clock::now();
  while (1)
  {
    auto current_time = std::chrono::high_resolution_clock::now();
    if (current_time - update_time > LED_TIME)
    {
      update_time = current_time;
      gpio_put(LED_PIN, !gpio_get(LED_PIN));
    }
    // tinyusb device task
    tud_task();

    if (!tud_hid_ready())
      continue;
    hid_task();
  }
}

template <size_t N>
static std::array<uint8_t, N> xfer(std::array<uint8_t, N> input)
{
  std::array<uint8_t, N> output{};
  spi_write_read_blocking(SPI_INSTANCE, input.data(), output.data(), N);
  return output;
}

static void hid_task_button(hid_gamepad_report_t &report)
{
  for (auto [pin, gamepad_btn] : DIGITAL_MAPPINGS)
  {
    if (gpio_get(pin))
    {
      report.buttons |= gamepad_btn;
    }
  }
  report.buttons = ~report.buttons;
}

static void hid_task_analog(hid_gamepad_report_t &report)
{
  std::array<uint8_t, 3> input, output;
  for (auto [channel, stick] : ANALOG_MAPPINGS)
  {
    input = mcp3008::encode(channel);
    output = xfer(input);
    uint32_t sensor_result = mcp3008::decode(output);
    int8_t joystick_result = static_cast<int8_t>(sensor_result / 4 - 128);
    switch (stick)
    {
    case left_x:
      report.x = joystick_result;
      break;
    case left_y:
      report.y = joystick_result;
      break;
    case left_z:
      report.z = joystick_result;
      break;
    case right_x:
      report.rx = joystick_result;
      break;
    case right_y:
      report.ry = joystick_result;
      break;
    case right_z:
      report.rz = joystick_result;
      break;
    default:
      continue;
    }
  }
}

static bool hid_report_is_same(const hid_gamepad_report_t &a, const hid_gamepad_report_t &b)
{
  // Ensure that the gamepad report is packed. We cannot have random data between members.
  static_assert(std::has_unique_object_representations<hid_gamepad_report_t>::value,
                "hid_gamepad_report_t is not packed");
  return std::memcmp(&a, &b, sizeof(hid_gamepad_report_t)) == 0;
}

static void hid_task()
{
  static hid_gamepad_report_t old_report{};
  hid_gamepad_report_t report{};

  hid_task_button(report);
  hid_task_analog(report);
  if (!hid_report_is_same(report, old_report))
  {
    tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
    old_report = report;
  }
}

// The following functions are callbacks that must be defined but we dont use them.

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen)
{
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {}

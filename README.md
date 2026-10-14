# STM32 CAN Motor Speed Control

Two STM32F103 boards on a CAN bus. One reads a potentiometer and broadcasts a
speed command; the other receives it and drives a servo/ESC by changing its PWM
pulse width. Turning the pot on one board changes the motor on the other.

## Layout

```
cantx/    transmitter — ADC → CAN frame
canrx/    receiver    — CAN frame → PWM
```

Both are STM32CubeIDE projects for the **STM32F103** (Blue Pill class), running
at **72 MHz** from an 8 MHz HSE through PLL ×9.

## Protocol

A single standard-ID frame carries the whole command.

| Field | Value |
|---|---|
| Std ID | `0x6A5` |
| DLC | 8 |
| Type | Data frame, standard ID |
| Payload | speed as `int8_t` in **byte 7**; bytes 0–6 unused |

### Transmitter (`cantx`)

Polls ADC1 (12-bit, 0–4095) and maps it to a signed range:

```c
calculatedValue = (int8_t)( ((uint32_t)readValue * 200) / 4095 - 100 );
```

So the pot sweeps **−100 … +100** — full reverse through stop to full forward.
The cast to `uint32_t` before multiplying by 200 is deliberate: `4095 * 200`
overflows a 16-bit intermediate. Sends once per second.

### Receiver (`canrx`)

Filters in hardware so the CPU only wakes for the right ID:

```c
sFilterConfig.FilterIdHigh     = 0x6A5 << 5;   // ID in the upper 11 bits
sFilterConfig.FilterMaskIdHigh = 0x7FF << 5;   // match all 11 ID bits
sFilterConfig.FilterMode       = CAN_FILTERMODE_IDMASK;
sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
```

The `<< 5` is required — the STM32 bxCAN filter registers hold a standard ID
left-aligned in the top 11 bits of a 16-bit half-register. Getting this wrong is
the classic bxCAN bug: the filter silently matches nothing.

Reception is interrupt-driven via `HAL_CAN_ActivateNotification(...,
CAN_IT_RX_FIFO1_MSG_PENDING)`. **`while(1)` in `main` is empty** — all the work
happens in `HAL_CAN_RxFifo1MsgPendingCallback`:

```c
uint32_t pulseWidth = 1500 + (receivedSpeed * 5);   // µs
if (pulseWidth > 2000) pulseWidth = 2000;
if (pulseWidth < 1000) pulseWidth = 1000;
__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulseWidth);
```

That's standard RC servo timing: 1000 µs full reverse, 1500 µs neutral, 2000 µs
full forward. ±100 × 5 µs lands exactly on the endpoints, and the clamps guard
against a malformed frame commanding an out-of-range pulse.

## Wiring

Each board needs a CAN transceiver (MCP2551/SN65HVD230 or similar) — the F103's
bxCAN peripheral provides `CAN_TX`/`CAN_RX` logic levels, not bus levels.
`CANH`/`CANL` between the two transceivers, **120 Ω termination at both ends**,
and a common ground.

Transmitter: potentiometer wiper to the ADC1 input.
Receiver: TIM2 channel 1 to the servo/ESC signal line.

## Build

Open `cantx/` and `canrx/` as separate projects in STM32CubeIDE, build, and
flash to the two boards. The `.ioc` files carry the CubeMX configuration if you
want to regenerate.

## Notes and limitations

- **No bus error handling.** Nothing checks the CAN error counters or handles
  bus-off, so a wiring fault fails silently.
- **No timeout on the receiver.** If the transmitter dies, the last commanded
  pulse width is held indefinitely — the motor keeps running. A real system
  would fail safe to neutral after a missed-frame deadline.
- Bytes 0–6 of the payload are transmitted uninitialised.
- The `HAL_Delay(1000)` transmit rate is far slower than CAN can manage; it's
  set for watching the demo, not for control bandwidth.

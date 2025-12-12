F.R.E.D. – Fire Retarding Experimental Device

F.R.E.D. is an autonomous Arduino-based robot that detects a heat source, drives toward it, and activates a water pump to simulate extinguishing a fire. The system uses an IR sensor mounted on a servo for directional scanning, ultrasonic distance sensing for range detection, and a relay-controlled pump for water deployment. A kill-switch via IR remote provides manual override at any time.

**Features**

Autonomous heat-seeking behavior

IR directional heat scanning

Ultrasonic distance sensing

Differential-drive movement

Automatic water spraying

Kill-switch override

Decorative AXEIT post-spray sequence


**Hardware**

Arduino (Nano/Uno)

Motor driver (H-bridge)

IR analog heat sensor → A2

HC-SR04 ultrasonic sensor → TRIG 12, ECHO 13

Scanning servo → 9

Axe servo → A1

Pump relay → 10

DC motors + external motor battery

Separate pump battery

IR remote + receiver → 3


**How It Works**

IR remote enables autonomous mode.

Servo sweeps IR sensor to find hottest angle.

Motors turn and drive toward that direction.

Ultrasonic sensor checks distance.

When close enough, robot aims, sprays water, and performs AXEIT.

Kill-switch immediately stops all motion.


**Contributors**

Owen Schulz – Lead programming, presentation

Sam Mason – Lead wiring & hardware integration, report writing

Both participated in testing, debugging, and design decisions.

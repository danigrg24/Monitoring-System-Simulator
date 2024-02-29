This C application simulates a traffic monitoring system in a tunnel equipped with smoke and gas sensors, as well as vehicle density sensors to control traffic flow.

Features:
Utilizes various synchronization mechanisms for efficient operation.
Refactored code for improved readability using an open-source Arduino library.

Components:

1) Vehicle Producer:
Simulates vehicle entry and exit from the tunnel.
Produces vehicle count information and updates a buffer.

2) Entry/Exit Monitor:
Monitors entry/exit sensors and updates the vehicle count.
Triggers an incident and halts the system if vehicle limit is exceeded.

3) Gas Sensor Monitor:
Monitors a gas sensor and triggers an incident if a preset limit is exceeded.

4) Smoke Sensor Monitor:
Monitors smoke sensors and triggers an incident upon detection of smoke.

5) Panic Button:
Monitors panic button status and triggers an incident upon activation.

6) External Operator:
Monitors external operator instructions (e.g., block entry/exit) and triggers corresponding incidents.

Synchronization:
Employs synchronization mechanisms to manage access to shared resources and prevent conflicts between tasks.


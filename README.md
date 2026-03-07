# TVC-EDF
Starting a project on Thrust Vector Controlled Electric Ducted Fan. This might take a while :/

Thrust vector controlled electric ducted fan

Purpose of this project:

- Personal enjoyment
- Stay relatively sharp with engineering design process and analytical side of things
- Learn about control theory
- Actually see a project through to its end
- Possibly convert this into a research paper or something we can use for applying to grad school later

General Requirements:

- UAS powered primarily through EDF/s
- Able to hover without leaving a 2x2x2 box for at least 1 minute
- Able to takeoff and land autonomously
- Can translate position 1.5 m with an accuracy of +/- 1’
- Reasonable cost. IDK $500 bucks?
    - :/ whelp will prob go over this significantly

Milestones and Timeline:

- Brainstorming and Selection
    - Jan 24
- Basic component layout and electronics schematic
    - Jan 31
- Model in CAD and basic code structure
    - Feb 7
- Physcially built and components check
    - Feb 21
- First Test
    - Feb 28 lmao

Systems:

- Computer
- Power
- Thrust
- Control actualizers
- Sensors
- Structure
- Communications
- Ground Control
- Landing Stabilizer

Brainstorming (Best implementation, creative ideas for improving development process):

- Computer
    - Teensy 4.1, already have
    - Run computations off laptop and send sensor data/commands back and forth
    - Raspberry Pi
    - Create custom board with embedded microcontrollers
- Power
    - 2 4S Lipos
    - 3+ Lipos for better spacing
    - 1+ 6S Lipo for EDF power
    - Larger capacity 4S Lipos
    - idk other types of batteries?
    - Step-down buck converter for powering other electronics
    - The other type of voltage regulator for the servos and the computer/sensors
- Thrust
    - Already have 70mm EDFs
        - HUH? apparently thrust ring increases thrust significantly
    - Idk this is kidna set in stone. If we need more thrust, be easier to remove weight than buy larger edf
- Control actualizers
    - 4 individually controlled servos
        - have redundant encoders? Anti-backlash system
            - springs to apply pressure?
        - May want to get a little higher quality digital ones
    - The vanes themselves
        - airfoil or wedge shaped?
        - Does it ahve a horizontal component of lift? or simply redirecting airflow and newtons 3rd alw
            - would this mean that the vanes can stall
    - reaction wheel, for just countering precession when significantly deflecting
    - could try to use 2 servos and somehow use a reaction for rotating along with precession countering
    - try multiple TVC controlled EDFs, stacked edfs?
    - 
- Sensors
    - Dont think much is needed
    - redundant IMUs
    - distance sensor later on for landing, t/o perhaps
    - tachometer for RPM measurement of edf?
- Structure
    - 3d printed
        - can try more robust materials or go super light and just print everything in like lw
            - maybe try tpu for vibration isolation
    - carbon fiber or alternative composite for more structural integrity
- Communications
    - 2 way radio, one onboard, other for ground station which will also have real time control alongside the telemetry
        - unless we can’t get a fast enough response rate. then we’ll use seaprate radios for data and manual control
    - don’t need something long range, just fast cycles rn
- Ground Control
    - probably another microcontroller that can receive the radio and perhaps send serial to a visualizer/control center like a laptop
    - Will need to build own visualizer and control center probably
- Landing Stabilizer
    - best probably simple and use passive support structures or legs
- Testing
    - For more precise movements, will probably need a gimbal system
    - Not sure I want to hook up batteries to it during testing
        - having constnat power won’t be accurate to it with batteries - would be important to have it light enough that it doesn’t need full throttle to hover - light as possible

What we actually need to do.

1. I want simulation/physics backed control. This probably means FBDs and some kind of system model with real factors
2. Ground control center. Realtime visualization of telemetry and ability to send commands. Redundant data storing.
3. Design, build, and assemble physical device. Actual intention or reason behind the dimensions or designs used instead of eyeballing. Actually getting numbers to line up with things as simple as thrust to weight. 
    1. “improve the lfit of an EDF is the Coanda effect”, thrust tubes, inlet tubes
4. Onboard Software. Nothing much, just faster rate, data logging unless most can be sent, able to try multiple types of control I guess. Maybe ability to be updatable through GCC - use the microSD for control constants or smth idk then restart or have it reload values.
    1. Checkout angular velocity feed forward controller
    2. I think goal is minimum 250 Hz
5. Ground test system. Gimballing, prob rotating mounting system to hold in place, load cell?, stable power source to not deal with battery. 

Other successes:

- bresh9019
- michael bruckner

1. Start with 4 and 2 without control portion. Already have base state machine and sensor input, can uncomment wireless portions and have everything on one board
2. Then start 1 and slowly portions of 3
3. Incorporate control into 4 and finish 3
4. See how well we can do things from the get go but probably try to get 5 to start testing

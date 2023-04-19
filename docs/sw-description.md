# Software description

The purpose of this document is to describe certain functions, custom definitions or the logic to achieve a concrete
behavior.

## Software definitions

Date, time, day of week ***parameters***: this variable refers to the day, month, year for the date; hours, minutes,
seconds for the time; and the name for day of the week.

## Software logic

### Main loop

In the ***main loop*** the software is in *Display clock mode*. In this mode the following tasks are executed:
- Request the status of both buttons, edit and set.
- Update the time and date on the LCD.
- If the button *edit* was pressed, then go to *Edition mode*.

### Logic to edit date and time.

Once the software is in *Edition mode*, it starts blinking the first parameter, which in this case is the day.
The software waits to receives a clicking event from one of the buttons. In case the *edit* button was clicked, then
the next parameter starts blinking. Otherwise, if the *set* button was clicked, then the software updates the value of
current parameter.

### Blinking animation effect

The blinking effect is achieved by alternating between a blank space or the parameter when displaying the string on the
LCD. The software waits before changing the string. This creates the animation of the parameter blinking when is
selected.

The following pseudocode describes the logic to create the blinking animation:

- Create a global variable that is going to be use as a timer to keep track of time.
- A flag is needed to indicate if the software has to display an empty string or the parameter's string.
- If the timer hasn't timed-out then do nothing.
- If timer has timed-out then evaluate the flag:
    - Reset timer.
    - Switch the value of the flag (it altarnates between 0 and 1).
    - Most of the parameters consist of 2 digits, however the day of the week consists of 3. If current parameter is
    not the day of the week, then set the empty string to 2 spaces, otherwise set it to 3 spaces.
    - Set the LCD cursor at the beginning of the parameter.
    - If flag == 0 display the blank space, otherwise display the parameter.
# Maryfish
Arduino control of Billy Bass Fish, playing a snippet of Proud Mary

This project consists of the following major parts:
1. Billy Bass Fish with 3 motors (Head, Tail, Body)
2. Arduino microcontroller
3. DFPlayer Arduino-compatible MP3 player
4. 3 NPN transistors (one for each motor)
5. Momentary switch
6. Micro SD card
7. MP3 file (Proud Mary)
8. Arduino program to play the MP3 file and move the fish motors


# Files in this project
1. `1_arduino_mp3.jpg` Image of arduino part of circuit
2. `2_transistor.jpg` Image of transistor/motor part of circuit (only 1 motor connection shown)
3. `demo.mov` Sample of fish playing and moving
4. `maryfish.ino` Arduino code to play the song and move the fish motors

# General approach to reproduce this project
1. Tear apart fish, access the Head, Tail, Body motor wires
2. Identify which motor wires are + and -
3. Connect DFPlayer, transistors, switch to Arduino
4. Load MP3 file onto a micro SD card and insert card into DFPlayer (see the ino file for notes on formatting the card and naming the file)
5. Load ino file into Arduino

# Other notes
1. The MP3 file was shortened from 4 minutes to 1 minute using the audio program, Audacity
2. A fade-out effect was added to the MP3 file using Audacity

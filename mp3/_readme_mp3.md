You need a lot of mp3 files for the different hours and minutes to be said. Here is what is required, how they needs to be stored and how to easily create them:

MP3 Voice output via DFPlayer module
------------------------------------
Attention: On microSD card, all sound files have to be put in a folder called "mp3"

The following files are expected:
1.mp3 - 24-mp3  : "zero hours", "one hours", "two hours" to "23 hours"; In German it is required to say "Uhr" meaning "hours" between hours and minutes of a timestamp!
25.mp3 - 84.mp3 : "zero" to "fifty-nine" for the minutes
85.mp3          : "to"
86.mp3          : "past"
87.mp3          : "quarter"
88.mp3          : "half"
85.mp3 - 88.mp3 are meant for colloquial time telling (future expansion, not yet implemented!)

Decent mp3 files can easily be created online using the following strategy:

1. Create a text file containing all required words with breaks, like in German: (included in github repository for your convenience)
  0 Uhr, 1 Uhr, 2 Uhr, 3 Uhr, 4 Uhr, 5 Uhr, 6 Uhr, 7 Uhr, 8 Uhr, 9 Uhr, 10 Uhr, 11 Uhr, 12 Uhr, 13 Uhr, 14 Uhr, 15 Uhr, 16 Uhr, 17 Uhr, 18 Uhr, 19 Uhr, 20 Uhr, 21 Uhr, 22 Uhr, 23 Uhr, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, Nach, Vor, Viertel, Halb,
2. Choose an online Text-To-Speech Service like https://www.naturalreaders.com/online/
3. Paste the text file contents to the input box
4. Choose language and voice
5. Use audacity for recording the output directly as explained under http://manual.audacityteam.org/man/tutorial_recording_computer_playback_on_windows.html
6. Label recording into separate tracks as described at http://manual.audacityteam.org/man/silence_finder_and_sound_finder.html
7. Export separate sound files from labels as described here: http://manual.audacityteam.org/man/splitting_a_recording_into_separate_tracks.html
8. Now click "ok" 88 times ;-) -> the resulting files are cut and named correctly by default.
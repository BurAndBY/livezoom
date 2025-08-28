# LiveZoom
![A screenshot of a Minecraft window with 2 windows enlarging the entity count part of the f3 menu and the piechart for easier viewing](https://i.ibb.co/B53rgJbw/image.png)
A simple program to zoom in on specific parts of the screen. Meant mostly for minecraft speedrunners who don't have enough PC power to run OBS.

# Building
Run `gcc main.c -o magnifier.exe -lgdi32 -mwindows` on a windows PC with MSYS2.
This was meant as a simple app for a simple purpose so it doesn't even have a Visual Studio project for it.

## Running
Run `magnifier.exe` and then Alt+Tab to Minecraft and select the part of the screen with the entity counter / piechart on it and then move the window somewhere where it doesn't obstruct your view. Changing the zoom level is only possible through the code for now.

# W Menu: get it onto Geode mobile

## 1. Build it on GitHub (free, no PC needed)
1. Make a free GitHub account and create a new empty repository.
2. Upload this project's files to the repo root, keeping the folder layout:
   - `mod.json`
   - `CMakeLists.txt`
   - `src/Settings.hpp`, `src/WMenu.hpp`, `src/WMenu.cpp`, `src/main.cpp`
   - `.github/workflows/build.yml`
   (On a phone browser, use "Add file -> Create new file", type the path such as
   `src/main.cpp` as the file name, and paste the contents.)
3. Open the repo's **Actions** tab. The "Build Mod" run starts by itself.
   Wait until it turns green (a few minutes).
4. Open the finished run, scroll to **Artifacts**, download **Build Output**.
   Unzip it; inside is `yourname.w-menu.geode`.

## 2. Install on Android
Move the `.geode` file to:

`/storage/emulated/0/Android/media/com.geode.launcher/game/geode/mods/`

Open Geometry Dash from the Geode launcher. The green **W** button is in the
main menu's bottom bar and in the pause menu.

## If the build fails
Open the failed run, copy the red error lines, and send them to Claude.

#include "SoundManager.h"

#include <thread>
#include <chrono>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>

#if __APPLE__
#include <mach-o/dyld.h>
#endif

std::string SoundManager::getExecutableDirectory() {
    char buffer[PATH_MAX];

#if __APPLE__
    // macOS: use _NSGetExecutablePath
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        std::string path(buffer);
        return path.substr(0, path.find_last_of('/'));
    }
    return "";
#else
    // Linux: read \proc\self\exe
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        std::string path(buffer);
        return path.substr(0, path.find_last_of('/'));
    }
    return "";
#endif
}

std::string SoundManager::soundPath(const std::string& filename) {
    // Sounds are expected in a \sounds\ subdirectory next to the executable.
    return getExecutableDirectory() + "/sounds/" + filename;
}

const char* SoundManager::getBackgroundSoundFile()      { return "background_sound_01.wav"; }
const char* SoundManager::getSoftDropSoundFile()        { return "soft_drop_2.wav"; }
const char* SoundManager::getHardDropSoundFile()        { return "hard_drop.wav"; }
const char* SoundManager::getLockPieceSoundFile()       { return "lock_piece.wav"; }
const char* SoundManager::getLineClearSoundFile()       { return "line_clear.wav"; }
const char* SoundManager::getFourLinesClearSoundFile()  { return "4lines_clear.wav"; }
const char* SoundManager::getLevelUpSoundFile()         { return "level_up.wav"; }
const char* SoundManager::getGameOverSoundFile()        { return "game_over.wav"; }

void SoundManager::playBackgroundSound() {
    std::string path = soundPath(getBackgroundSoundFile());

#if __APPLE__
    // Loop background wav using afplay (macOS)
    std::string cmd = "while true; do afplay \"" + path + "\"; done &";
#else
    // Linux: loop background wav using aplay
    std::string cmd = "while true; do aplay -q \"" + path + "\"; done &";
#endif
    system(cmd.c_str());
}

void SoundManager::stopBackgroundSound() {
#if __APPLE__
    // Kill any afplay process playing our background file
    system("pkill -f \"afplay.*background_sound_01.wav\" >/dev/null 2>&1");
#else
    // Kill any aplay process playing our background file
    system("pkill -f \"aplay.*background_sound_01.wav\" >/dev/null 2>&1");
#endif
}

void SoundManager::playSFX(const std::string& filename) {
    std::string path = soundPath(filename);

#if __APPLE__
    // Single shot sound playback in background
    std::string cmd = "afplay \"" + path + "\" &";
#else
    // Linux: choose player based on file extension.
    std::string ext = filename.substr(filename.find_last_of('.'));
    std::string cmd;

    if (ext == ".mp3") {
        // MP3: try mpg123, fall back to ffplay
        cmd =
            "(command -v mpg123 >/dev/null 2>&1 && mpg123 -q \"" + path + "\") || "
            "(command -v ffplay >/dev/null 2>&1 && "
            " ffplay -nodisp -autoexit -loglevel quiet \"" + path + "\") &";
    } else {
        // WAV: try aplay, fall back to ffplay
        cmd =
            "(command -v aplay >/dev/null 2>&1 && aplay -q \"" + path + "\") || "
            "(command -v ffplay >/dev/null 2>&1 && "
            " ffplay -nodisp -autoexit -loglevel quiet \"" + path + "\") &";
    }
#endif

    system(cmd.c_str());
}

void SoundManager::playSoundAfterDelay(const std::string& file, int delayMs) {
    // Fire\-and\-forget detached thread that waits then plays a sound.
    std::thread([file, delayMs]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        playSFX(file);
    }).detach();
}

void SoundManager::playSoftDropSound()   { playSFX(getSoftDropSoundFile()); }
void SoundManager::playHardDropSound()   { playSFX(getHardDropSoundFile()); }
void SoundManager::playLockPieceSound()  { playSFX(getLockPieceSoundFile()); }
void SoundManager::playLineClearSound()  { playSFX(getLineClearSoundFile()); }
void SoundManager::play4LinesClearSound(){ playSFX(getFourLinesClearSoundFile()); }
void SoundManager::playLevelUpSound()    { playSoundAfterDelay(getLevelUpSoundFile(), 1000); }
void SoundManager::playGameOverSound()   { playSFX(getGameOverSoundFile()); }

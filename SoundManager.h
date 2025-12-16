#pragma once
#include <string>

// \brief Simple sound manager using external system commands.
// Uses different players on macOS/Linux and assumes files under \sounds\.
class SoundManager {
public:
    // \brief Start background music in a looping background process.
    static void playBackgroundSound();

    // \brief Stop background music processes started earlier.
    static void stopBackgroundSound();

    // \brief Play soft drop effect.
    static void playSoftDropSound();

    // \brief Play hard drop effect.
    static void playHardDropSound();

    // \brief Play piece lock effect.
    static void playLockPieceSound();

    // \brief Play line clear effect (1\-3 lines).
    static void playLineClearSound();

    // \brief Play Tetris (4 line clear) effect.
    static void play4LinesClearSound();

    // \brief Play level up effect (delayed slightly).
    static void playLevelUpSound();

    // \brief Play game over effect.
    static void playGameOverSound();

private:
    static std::string getExecutableDirectory();
    static std::string soundPath(const std::string& filename);
    static void playSFX(const std::string& filename);
    static void playSoundAfterDelay(const std::string& file, int delayMs);

    // Explicit filenames, kept small for clarity.
    static const char* getBackgroundSoundFile();
    static const char* getSoftDropSoundFile();
    static const char* getHardDropSoundFile();
    static const char* getLockPieceSoundFile();
    static const char* getLineClearSoundFile();
    static const char* getFourLinesClearSoundFile();
    static const char* getLevelUpSoundFile();
    static const char* getGameOverSoundFile();
};

#pragma once
#include <string>

class SoundManager {
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
public:
    // Start background music in a looping background process.
    static void playBackgroundSound();

    // Stop background music processes started earlier.
    static void stopBackgroundSound();

    // Play soft drop effect.
    static void playSoftDropSound();

    // Play hard drop effect.
    static void playHardDropSound();

    // Play piece lock effect.
    static void playLockPieceSound();

    // Play line clear effect (1\-3 lines).
    static void playLineClearSound();

    // Play Tetris (4 line clear) effect.
    static void play4LinesClearSound();

    // Play level up effect (delayed slightly).
    static void playLevelUpSound();

    // Play game over effect.
    static void playGameOverSound();
};

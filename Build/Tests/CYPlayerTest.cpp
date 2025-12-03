#include <iostream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "CYPlayer/ICYPlayer.hpp"
#include "CYPlayer/CYPlayerFactory.hpp"
#include "CYPlayer/CYPlayerDefine.hpp"

// Sleep function that works on all platforms
void sleep_ms(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

// 回调函数实现
void PlayerEventCallback(const cry::EPlayerEventInfo* eventInfo) {
    if (!eventInfo) {
        std::cout << "Event received with null event info" << std::endl;
        return;
    }
    
    switch (eventInfo->eEventType) {
        case cry::TYPE_EVENT_PLAYBACK_FINISHED:
            std::cout << "Playback finished" << std::endl;
            break;
        case cry::TYPE_EVENT_ERROR_OCCURRED:
            std::cout << "Error occurred: " << eventInfo->nErrorCode << " - " << eventInfo->szMessage << std::endl;
            break;
        case cry::TYPE_EVENT_BUFFERING_STARTED:
            std::cout << "Buffering started" << std::endl;
            break;
        case cry::TYPE_EVENT_BUFFERING_ENDED:
            std::cout << "Buffering ended" << std::endl;
            break;
        default:
            std::cout << "Unknown event: " << static_cast<int>(eventInfo->eEventType) << std::endl;
            break;
    }
}

void PlayerStateCallback(cry::EStateType state) {
    switch (state) {
        case cry::TYPE_STATUS_IDLE:
            std::cout << "Player status: IDLE" << std::endl;
            break;
        case cry::TYPE_STATUS_INITIALIZED:
            std::cout << "Player status: INITIALIZED" << std::endl;
            break;
        case cry::TYPE_STATUS_PREPARED:
            std::cout << "Player status: PREPARED" << std::endl;
            break;
        case cry::TYPE_STATUS_PLAYING:
            std::cout << "Player status: PLAYING" << std::endl;
            break;
        case cry::TYPE_STATUS_PAUSED:
            std::cout << "Player status: PAUSED" << std::endl;
            break;
        case cry::TYPE_STATUS_STOPPED:
            std::cout << "Player status: STOPPED" << std::endl;
            break;
        case cry::TYPE_STATUS_COMPLETED:
            std::cout << "Player status: COMPLETED" << std::endl;
            break;
        case cry::TYPE_STATUS_ERROR:
            std::cout << "Player status: ERROR" << std::endl;
            break;
        default:
            std::cout << "Unknown state: " << static_cast<int>(state) << std::endl;
            break;
    }
}

void PlayerPositionCallback(int64_t position, int64_t duration) {
    std::cout << "Position: " << position << " / " << duration << " seconds" << std::endl;
}

void PlayerLogCallback(cry::ELogType logType, const char* message, const char* file, const char* location, int line) {
    switch (logType) {
        case cry::TYPE_LOG_DEBUG:
            std::cout << "[DEBUG] ";
            break;
        case cry::TYPE_LOG_INFO:
            std::cout << "[INFO] ";
            break;
        case cry::TYPE_LOG_WARNING:
            std::cout << "[WARNING] ";
            break;
        case cry::TYPE_LOG_ERROR:
            std::cout << "[ERROR] ";
            break;
        case cry::TYPE_LOG_FATAL:
            std::cout << "[FATAL] ";
            break;
        default:
            std::cout << "[UNKNOWN] ";
            break;
    }
    std::cout << message << " (" << file << ":" << line << ")" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <media_file_path>" << std::endl;
        return 1;
    }

    const char* mediaPath = argv[1];
    std::cout << "Opening media file: " << mediaPath << std::endl;

    try {
        // 创建播放器实例
        cry::ICYPlayer* player = cry::CYPlayerFactory::CreatePlayer();
        
        if (!player) {
            std::cerr << "Failed to create player instance" << std::endl;
            return 1;
        }
        
        // 设置播放器回调
        player->SetEventCallback(PlayerEventCallback);
        player->SetStateCallback(PlayerStateCallback);
        player->SetPositionCallback(PlayerPositionCallback);
        player->SetLogCallBack(PlayerLogCallback);

        // 设置播放器参数
        cry::EPlayerParam playerParam;
        playerParam.eClockType = cry::TYPE_SYNC_CLOCK_AUDIO;
        playerParam.eVideoRenderType = cry::TYPE_VIDEO_RENDER_SDL;
        playerParam.eAudioRenderType = cry::TYPE_AUDIO_RENDER_SDL;
        
        // 初始化播放器
        int16_t ret = player->Init(&playerParam);
        if (ret != cry::ERR_SUCESS) {
            std::cerr << "Failed to initialize player: " << ret << std::endl;
            cry::CYPlayerFactory::DestroyPlayer(player);
            return 1;
        }
        
        // 设置渲染窗口
        #ifdef _WIN32
        // 在实际应用中，您可能需要从GUI框架获取窗口句柄
        // player->SetWindow(hWnd);
        #endif

        // 打开媒体文件
        cry::EPlayerMediaParam mediaParam;
        mediaParam.nStartVolume = 100;
        mediaParam.nLoop = 1;
        
        ret = player->Open(mediaPath, &mediaParam);
        if (ret != cry::ERR_SUCESS) {
            std::cerr << "Failed to open media: " << ret << std::endl;
            cry::CYPlayerFactory::DestroyPlayer(player);
            return 1;
        }

        // 获取媒体信息
        std::cout << "Media Information:" << std::endl;
        std::cout << "  Duration: " << player->GetDuration() << " ms" << std::endl;
        
        // 开始播放
        ret = player->Play();
        if (ret != cry::ERR_SUCESS) {
            std::cerr << "Failed to start playback: " << ret << std::endl;
            cry::CYPlayerFactory::DestroyPlayer(player);
            return 1;
        }

        // 播放10秒然后暂停
        std::cout << "Playing for 10 seconds..." << std::endl;
        sleep_ms(10000);
        
        bool paused = false;
        player->Pause(&paused);
        std::cout << "Paused for 2 seconds... Paused state: " << (paused ? "true" : "false") << std::endl;
        sleep_ms(2000);
        
        // 恢复播放
        player->Play();
        std::cout << "Resumed playback..." << std::endl;
        
        // 再播放5秒然后跳转
        sleep_ms(5000);
        std::cout << "Seeking to 30 seconds..." << std::endl;
        player->Seek(30000); // 毫秒为单位
        
        // 设置音量 (0.0 ~ 1.0)
        player->SetVolume(0.8f);
        std::cout << "Current volume: " << player->GetVolume() << std::endl;
        
        // 再播放10秒
        sleep_ms(10000);
        
        // 停止并清理资源
        player->Stop();
        player->UnInit();
        cry::CYPlayerFactory::DestroyPlayer(player);
        std::cout << "Player stopped and resources released" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 
#pragma once
/*
    �������� Api - https://core.telegram.org/bots/api
    ����� = token
    ������������� ���� - id
    ����� - UTF-8

    ��������� ��������� https://api.telegram.org/bot{token}/sendMessage?chat_id={chatid}text=test
    ��������� ���������� https://api.telegram.org/bot{token}/getUpdates
*/

#ifdef CPPDLL_EXPORTS
    #define DLLIMPORT_EXPORT __declspec(dllexport)
#else
    #define DLLIMPORT_EXPORT __declspec(dllimport)
#endif

#include <map>
#include <memory>
#include <string>
#include <functional>
#include <list>

#include <tgbot/tgbot.h>

// ����������� ������ � UTF-8
inline std::string DLLIMPORT_EXPORT getUtf8Str(const CString& str) { return std::string(CW2A(str, CP_UTF8)); }

// ����������� ������ UTF-8 � CString
inline CString DLLIMPORT_EXPORT getUNICODEString(const std::string& utf8Str);

//----------------------------------------------------------------------------//
// ��������� ������������ ��� ��������� ���������� �� �������� ����
interface DLLIMPORT_EXPORT ITelegramAllerter
{
    virtual ~ITelegramAllerter() = default;
    // ���������� � ��������� ������ � ������ �������� ����
    virtual void onAllertFromTelegram(const CString& allertMessage) = 0;
};

typedef TgBot::Message::Ptr MessagePtr;
typedef TgBot::EventBroadcaster::MessageListener CommandFunction;
//----------------------------------------------------------------------------//
interface DLLIMPORT_EXPORT ITelegramThread
{
    // �������� ����������� ������ ����� �������� ��������������� ����� ��-�� ������������
    // longpoll ���������, TODO ���������� �� ���� BoostHttpOnlySslClient
    virtual ~ITelegramThread() = default;

    // ������ ������
    virtual void startTelegramThread(const std::map<std::string, CommandFunction>& commandsList,
                                     const CommandFunction& onUnknownCommand = nullptr,
                                     const CommandFunction& onNonCommandMessage = nullptr) = 0;

    // ��������� ������
    virtual void stopTelegramThread() = 0;

    // ������� �������� ��������� � ����
    virtual void sendMessage(const std::list<int64_t>& chatIds, const CString& msg,
                             bool disableWebPagePreview = false, int32_t replyToMessageId = 0,
                             TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
                             const std::string& parseMode = "", bool disableNotification = false) = 0;

    // ������� �������� ��������� � ���
    virtual void sendMessage(int64_t chatId, const CString& msg, bool disableWebPagePreview = false,
                             int32_t replyToMessageId = 0,
                             TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
                             const std::string& parseMode = "", bool disableNotification = false) = 0;

    // ���������� ������� ���� ����� ������ ��� ������������
    virtual TgBot::EventBroadcaster& getBotEvents() = 0;

    // ��������� ��� ����
    virtual const TgBot::Api& getBotApi() = 0;
};
typedef std::unique_ptr<ITelegramThread> ITelegramThreadPtr;

// ������� ��������� ������ ������
inline DLLIMPORT_EXPORT
ITelegramThreadPtr CreateTelegramThread(const std::string& token,
                                        ITelegramAllerter* allertInterface = nullptr);

// ������� ��������� �������� ����
inline DLLIMPORT_EXPORT
std::unique_ptr<TgBot::Bot> CreateTelegramBot(const std::string& token,
                                              const TgBot::HttpClient& client);

// ���������� ������� ���������� �� �������� ������
inline DLLIMPORT_EXPORT
void HandleTgUpdate(const TgBot::EventHandler& handler, TgBot::Update::Ptr update);

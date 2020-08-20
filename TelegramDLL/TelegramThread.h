#pragma once
/*
    Описание Api - https://core.telegram.org/bots/api
    токен = token
    Идентификатор чата - id
    Текст - UTF-8

    Отправить сообщение https://api.telegram.org/bot{token}/sendMessage?chat_id={chatid}text=test
    Проверить обновления https://api.telegram.org/bot{token}/getUpdates
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

// преобразуем строку в UTF-8
inline std::string DLLIMPORT_EXPORT getUtf8Str(const CString& str) { return std::string(CW2A(str, CP_UTF8)); }

// преобразуем строку UTF-8 в CString
inline CString DLLIMPORT_EXPORT getUNICODEString(const std::string& utf8Str);

//----------------------------------------------------------------------------//
// интерфейс используемый для получения оповещений из телеграм бота
interface DLLIMPORT_EXPORT ITelegramAllerter
{
    virtual ~ITelegramAllerter() = default;
    // оповещение о возникшей ошибке в работе телеграм бота
    virtual void onAllertFromTelegram(const CString& allertMessage) = 0;
};

typedef TgBot::Message::Ptr MessagePtr;
typedef TgBot::EventBroadcaster::MessageListener CommandFunction;
//----------------------------------------------------------------------------//
interface DLLIMPORT_EXPORT ITelegramThread
{
    // удаление работающего потока может занимать продолжительное время из-за особенностей
    // longpoll реквестов, TODO переделать на свой BoostHttpOnlySslClient
    virtual ~ITelegramThread() = default;

    // запуск потока
    virtual void startTelegramThread(const std::map<std::string, CommandFunction>& commandsList,
                                     const CommandFunction& onUnknownCommand = nullptr,
                                     const CommandFunction& onNonCommandMessage = nullptr) = 0;

    // остановка потока
    virtual void stopTelegramThread() = 0;

    // функция отправки сообщений в чаты
    virtual void sendMessage(const std::list<int64_t>& chatIds, const CString& msg,
                             bool disableWebPagePreview = false, int32_t replyToMessageId = 0,
                             TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
                             const std::string& parseMode = "", bool disableNotification = false) = 0;

    // функция отправки сообщения в чат
    virtual void sendMessage(int64_t chatId, const CString& msg, bool disableWebPagePreview = false,
                             int32_t replyToMessageId = 0,
                             TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
                             const std::string& parseMode = "", bool disableNotification = false) = 0;

    // возвращает события бота чтобы самому все обрабатывать
    virtual TgBot::EventBroadcaster& getBotEvents() = 0;

    // получение апи бота
    virtual const TgBot::Api& getBotApi() = 0;
};
typedef std::unique_ptr<ITelegramThread> ITelegramThreadPtr;

// создаем экземпляр нашего класса
inline DLLIMPORT_EXPORT
ITelegramThreadPtr CreateTelegramThread(const std::string& token,
                                        ITelegramAllerter* allertInterface = nullptr);

// создаем экземпляр телеграм бота
inline DLLIMPORT_EXPORT
std::unique_ptr<TgBot::Bot> CreateTelegramBot(const std::string& token,
                                              const TgBot::HttpClient& client);

// обработать событие обновления от телеграм канала
inline DLLIMPORT_EXPORT
void HandleTgUpdate(const TgBot::EventHandler& handler, TgBot::Update::Ptr update);

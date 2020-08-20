/*
    Описание Api - https://core.telegram.org/bots/api
    токен = token
    Идентификатор чата - id
    Текст - UTF-8

    Отправить сообщение https://api.telegram.org/bot{token}/sendMessage?chat_id={chat_id}&text=test
    Проверить обновления https://api.telegram.org/bot{token}/getUpdates
*/

#include "stdafx.h"

#include <string>
#include <thread>

#include "TelegramThread.h"

#include <tgbot/tgbot.h>

using namespace TgBot;

// структура с данными для работы потока
struct WorkTelegramData
{
    // флаг работы потока
    std::atomic_bool bThreadWorkFlag;
    // бот
    Bot bot;

#ifdef HAVE_CURL
    // клиент
    CurlHttpClient curlHttpClient;
#endif //HAVE_CURL

    // интерфейс оповещения об ошибке
    ITelegramAllerter* allertHelper;

    // конструктор
    WorkTelegramData(const std::string& token, ITelegramAllerter* allertInterface)
        : bot(token
#ifdef HAVE_CURL
              , curlHttpClient
#endif // HAVE_CURL
              )
        , allertHelper(allertInterface)
        , bThreadWorkFlag(false)
    {}
};

// чтобы не торчать наружу внутрянкой бота мы делаем вспомогательный класс
class TelegramThread : public ITelegramThread
{
public:
    // token - токен бота
    TelegramThread(const std::string& token,
                   ITelegramAllerter* allertInterface = nullptr);
    ~TelegramThread();

// ITelegramThread
public:
    // запуск потока
    void startTelegramThread(const std::map<std::string, CommandFunction>& commandsList,
                             const CommandFunction& onUnknownCommand = nullptr,
                             const CommandFunction& onNonCommandMessage = nullptr) override;
    // остановка потока
    void stopTelegramThread() override;

    // функция отправки сообщений в чаты
    void sendMessage(const std::list<int64_t>& chatIds, const CString& msg, bool disableWebPagePreview = false, int32_t replyToMessageId = 0,
                     GenericReply::Ptr replyMarkup = std::make_shared<GenericReply>(), const std::string& parseMode = "", bool disableNotification = false) override;

    // функция отправки сообщения в чат
    void sendMessage(int64_t chatId, const CString& msg, bool disableWebPagePreview = false, int32_t replyToMessageId = 0,
                     GenericReply::Ptr replyMarkup = std::make_shared<GenericReply>(), const std::string& parseMode = "", bool disableNotification = false) override;

    // возвращает события бота чтобы самому все обрабатывать
    TgBot::EventBroadcaster& getBotEvents() override;

    // получение апи бота
    const TgBot::Api& getBotApi() override;
public:
    // рабочий поток бота телеграма
    std::thread m_telegramThread;

    // данные необходимые для работы телеграма
    WorkTelegramData m_telegramWorkData;
};

//----------------------------------------------------------------------------//
// отправка оповещения об ошибке родителю
void sendAllert(ITelegramAllerter* allertHelper, const char *format, ...);
void sendAllert(ITelegramAllerter* allertHelper, const CString& str);

//----------------------------------------------------------------------------//
TelegramThread::TelegramThread(const std::string& token,
                               ITelegramAllerter* allertInterface /*= nullptr*/)
    : m_telegramWorkData(token, allertInterface)
{
}

//----------------------------------------------------------------------------//
TelegramThread::~TelegramThread()
{
    stopTelegramThread();

    if (m_telegramThread.joinable())
        m_telegramThread.join();
}

// рабочий поток
// commandsList - перечень команд и исполняемых функций
// onAnyMessageCommand - код выполняемый при получении любого сообщения
UINT telegramWorkThread(WorkTelegramData* telegramData)
{
    try
    {
        TRACE("Bot username: %s\n", telegramData->bot.getApi().getMe()->username.c_str());
        telegramData->bot.getApi().deleteWebhook();
    }
    catch (std::exception& e)
    {
        sendAllert(telegramData->allertHelper, "Ошибка инициализации бота, описание ошибки: %s\n", e.what());
    }

    TgLongPoll longPoll(telegramData->bot);
    while (telegramData->bThreadWorkFlag)
    {
        try
        {
            TRACE("Long poll started\n");
            longPoll.start();
        }
        catch (std::exception& e)
        {
            if (telegramData->bThreadWorkFlag)
                break;

            TRACE(L"error: %s\n", e.what());
            sendAllert(telegramData->allertHelper, "Ошибка в работе бота: %s\n", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    return 0;
}

//----------------------------------------------------------------------------//
void TelegramThread::startTelegramThread(const std::map<std::string, CommandFunction>& commandsList,
                                         const CommandFunction& onUnknownCommand /*= nullptr*/,
                                         const CommandFunction& onNonCommandMessage /*= nullptr*/)
{
    for (auto& command : commandsList)
    {
        m_telegramWorkData.bot.getEvents().onCommand(command.first, command.second);
    }

    m_telegramWorkData.bot.getEvents().onUnknownCommand(onUnknownCommand);
    m_telegramWorkData.bot.getEvents().onNonCommandMessage(onNonCommandMessage);

    m_telegramWorkData.bThreadWorkFlag = true;

    m_telegramThread = std::thread(&telegramWorkThread, &m_telegramWorkData);
}

//----------------------------------------------------------------------------//
void TelegramThread::stopTelegramThread()
{
    m_telegramWorkData.bThreadWorkFlag = false;
    m_telegramWorkData.allertHelper = nullptr;
}

//----------------------------------------------------------------------------//
void TelegramThread::sendMessage(const std::list<int64_t>& chatIds, const CString& msg,
                                 bool disableWebPagePreview, int32_t replyToMessageId,
                                 GenericReply::Ptr replyMarkup, const std::string& parseMode,
                                 bool disableNotification)
{
    // отправляем сообщение всем пользователям
    for (auto& chatId : chatIds)
    {
        try
        {
            m_telegramWorkData.bot.getApi().sendMessage(chatId, getUtf8Str(msg), disableWebPagePreview,
                                                        replyToMessageId, replyMarkup,
                                                        parseMode, disableNotification);
        }
        catch (std::exception& e)
        {
            TRACE("Error sendMessage: %s\n", e.what());
            sendAllert(m_telegramWorkData.allertHelper, "Ошибка отправки сообщения: %s\n", e.what());
        }
    }
}

//----------------------------------------------------------------------------//
void TelegramThread::sendMessage(int64_t chatId, const CString& msg,
                                 bool disableWebPagePreview, int32_t replyToMessageId,
                                 GenericReply::Ptr replyMarkup, const std::string& parseMode,
                                 bool disableNotification)
{
    std::list<int64_t> chatIds;
    chatIds.push_back(chatId);
    sendMessage(chatIds, msg, disableWebPagePreview,
                replyToMessageId, replyMarkup,
                parseMode, disableNotification);
}

//----------------------------------------------------------------------------//
TgBot::EventBroadcaster& TelegramThread::getBotEvents()
{
    return m_telegramWorkData.bot.getEvents();
}

//----------------------------------------------------------------------------//
const TgBot::Api& TelegramThread::getBotApi()
{
    return m_telegramWorkData.bot.getApi();
}

//----------------------------------------------------------------------------//
void sendAllert(ITelegramAllerter* allertHelper, const char *format, ...)
{
    if (!allertHelper)
        return;

    va_list arg;
    int len;
    char * orig_msg;

    /* Compute length of original message */
    va_start(arg, format);
    len = vsnprintf(NULL, 0, format, arg);
    va_end(arg);

    /* Allocate space for original message */
    orig_msg = (char *)calloc(len+1, sizeof(char));

    /* Write original message to string */
    va_start(arg, format);
    vsnprintf(orig_msg, len+1, format, arg);
    va_end(arg);

    allertHelper->onAllertFromTelegram(CString(orig_msg));

    free(orig_msg);
}

//----------------------------------------------------------------------------//
void sendAllert(ITelegramAllerter* allertHelper, const CString& str)
{
    if (allertHelper)
        allertHelper->onAllertFromTelegram(str);
}

//----------------------------------------------------------------------------//
CString getUNICODEString(const std::string& utf8Str)
{
    CString cstr;

    int utf8StrLen = (int)strlen(utf8Str.c_str());

    if (utf8StrLen == 0)
    {
        cstr.Empty();
        return cstr;
    }

    LPTSTR ptr = cstr.GetBuffer(utf8StrLen + 1);

#ifdef UNICODE
    // CString is UNICODE string so we decode
    int newLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), utf8StrLen, ptr, utf8StrLen + 1);
    if (!newLen)
    {
        cstr.ReleaseBuffer(0);
        return false;
    }
#else
    WCHAR* buf = (WCHAR*)malloc(utf8StrLen);

    if (buf == NULL)
    {
        cstr.ReleaseBuffer(0);
        return false;
    }

    int newLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), utf8StrLen, buf, utf8StrLen);
    if (!newLen)
    {
        free(buf);
        cstr.ReleaseBuffer(0);
        return false;
    }

    assert(newLen < utf8StrLen);
    newLen = WideCharToMultiByte(
        CP_ACP, 0,
        buf, newLen, ptr, utf8StrLen
    );
    if (!newLen)
    {
        free(buf);
        cstr.ReleaseBuffer(0);
        return false;
    }

    free(buf);
#endif

    cstr.ReleaseBuffer(newLen);
    return cstr;
}

//----------------------------------------------------------------------------//
inline DLLIMPORT_EXPORT ITelegramThreadPtr CreateTelegramThread(const std::string& token,
                                                                ITelegramAllerter* allertInterface)
{
    return std::make_unique<TelegramThread>(token, allertInterface);
}

//----------------------------------------------------------------------------//
inline DLLIMPORT_EXPORT std::unique_ptr<TgBot::Bot> CreateTelegramBot(const std::string& token,
                                                                      const TgBot::HttpClient& client)
{
    return std::make_unique<TgBot::Bot>(token, client);
}

//----------------------------------------------------------------------------//
inline void DLLIMPORT_EXPORT HandleTgUpdate(const TgBot::EventHandler& handler,
                                            TgBot::Update::Ptr update)
{
    handler.handleUpdate(update);
}

/*
// отправка запроса через CURL
#include "curl/curl.h"
size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void sendRequestByCurl()
{
    CURL* curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        const CStringW unicode1 = L"chat_id=358782858&text=тест"; // 'Alpha' and 'Omega'
        const CStringA utf8 = CW2A(unicode1, CP_UTF8);

        std::string readBuffer;
        curl_easy_setopt(curl, CURLOPT_URL,
                         "https://api.telegram.org/bot775186029:AAHh1StDVlvod-oLACt1NuHheJD5j7mTQ4M/sendMessage");
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, utf8);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}*/
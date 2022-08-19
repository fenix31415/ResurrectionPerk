extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
#ifndef DEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= Version::PROJECT;
	*path += ".log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef DEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

#include "ResurrectionAPI.h"

const char* plugin_name = "Daminikov_Perk.esp";
const int f314RM_Mgef = 0x813;
const int f314RM_Perk = 0x816;
const int f314RM_Spel = 0x815;
const int f314RM_Mesg = 0x817;

void debug_notification(RE::BGSMessage* msg)
{
	RE::BSString a;
	msg->GetDescription(a, msg);
	RE::DebugNotification(a.c_str());
}

class PerkResurrection : public ResurrectionAPI
{
	bool should_resurrect(RE::Actor* a) const override
	{
		static auto perk = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSPerk>(f314RM_Perk, plugin_name);
		static auto mgef = RE::TESDataHandler::GetSingleton()->LookupForm<RE::EffectSetting>(f314RM_Mgef, plugin_name);
		return a->HasPerk(perk) && !a->HasMagicEffect(mgef);
	}

	void resurrect(RE::Actor* a) override
	{
		float total = 1.0f * FenixUtils::get_total_av(a, RE::ActorValue::kHealth);
		float to_res = std::max(0.0f, total - a->GetActorValue(RE::ActorValue::kHealth));
		a->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, to_res);

		static auto spel = RE::TESDataHandler::GetSingleton()->LookupForm<RE::SpellItem>(f314RM_Spel, plugin_name);
		FenixUtils::cast_spell(a, a, spel);

		static auto mesg = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSMessage>(f314RM_Mesg, plugin_name);
		debug_notification(mesg);
	}
};

void addSubscriber()
{
	if (auto pluginHandle = GetModuleHandleA("ResurrectionAPI.dll")) {
		if (auto AddSubscriber = (AddSubscriber_t)GetProcAddress(pluginHandle, "ResurrectionAPI_AddSubscriber")) {
			AddSubscriber(std::make_unique<PerkResurrection>());
		}
	}
}

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message)
{
	switch (message->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		addSubscriber();

		break;
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	auto g_messaging = reinterpret_cast<SKSE::MessagingInterface*>(a_skse->QueryInterface(SKSE::LoadInterface::kMessaging));
	if (!g_messaging) {
		logger::critical("Failed to load messaging interface! This error is fatal, plugin will not load.");
		return false;
	}

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(1 << 10);

	g_messaging->RegisterListener("SKSE", SKSEMessageHandler);

	return true;
}

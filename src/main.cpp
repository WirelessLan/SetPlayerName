#include <fstream>
#include <string>

std::string ReplaceNewLines(const std::string& a_input) {
	std::string result;

	for (char c : a_input) {
		if (c == '\n' || c == '\r')
			continue;

		result += c;
	}

	return result;
}

std::string Trim(const std::string& a_str) {
	std::string result = a_str;

	result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
		return !std::isspace(ch);
		}));

	result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
		}).base(), result.end());

	return result;
}

const std::string g_configPath = fmt::format("Data\\F4SE\\Plugins\\{}.txt", Version::PROJECT);

std::string ReadConfig() {
	std::ifstream configFile(g_configPath);
	if (!configFile.is_open()) {
		logger::warn("Cannot open the config file: {}", g_configPath);
		return std::string{};
	}

	std::stringstream buffer;
	buffer << configFile.rdbuf();

	return Trim(ReplaceNewLines(buffer.str()));
}

std::string SetName(std::monostate) {
	auto playerName = ReadConfig();
	if (playerName.empty())
		return std::string{};

	RE::PlayerCharacter* pc = RE::PlayerCharacter::GetSingleton();
	if (!pc)
		return std::string{};

	RE::TESNPC* pNpc = pc->GetNPC();
	if (!pNpc)
		return std::string{};

	pNpc->fullName = playerName;

	return playerName;
}

bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* a_vm) {
	a_vm->BindNativeMethod("SetPlayerName"sv, "SetName_Native"sv, SetName);
	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface * a_f4se, F4SE::PluginInfo * a_info) {
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format("{}.log", Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("Global Log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::trace);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface * a_f4se) {
	F4SE::Init(a_f4se);

	const F4SE::PapyrusInterface* papyrus = F4SE::GetPapyrusInterface();
	if (papyrus)
		papyrus->Register(RegisterPapyrusFunctions);

	return true;
}

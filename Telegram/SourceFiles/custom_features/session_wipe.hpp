#pragma once

#include <QString>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QStandardPaths>
#include <atomic>

#include "custom_features/custom_settings.hpp"
#include "core/application.h"
#include "main/main_domain.h"
#include "main/main_account.h"
#include "logs.h"
#include "settings.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace CustomFeatures {

inline std::atomic<bool> IsWipingInProgress{ false };

inline void WipeSessionAndExit(bool relaunch = false) {
	bool expected = false;
	if (!IsWipingInProgress.compare_exchange_strong(expected, true)) {
		return;
	}

	// 1. Send logout requests to Telegram servers for all active accounts so auth keys are revoked.
	if (Core::IsAppLaunched()) {
		for (const auto &item : Core::App().domain().accounts()) {
			if (item.account) {
				item.account->logOut();
			}
		}
	}

#ifdef Q_OS_WIN
	const auto pid = QCoreApplication::applicationPid();
	const auto nativeExe = QDir::toNativeSeparators(cExeDir() + cExeName());
	const auto nativeWork = QDir::toNativeSeparators(QDir(cWorkingDir()).absolutePath());
	const auto customIni = QDir::toNativeSeparators(ClientConfig::getSettingsFilePath());

	const auto scriptPath = QDir::toNativeSeparators(
		QDir::tempPath() + u"/tg_wipe_cleanup.bat"_q);

	// Write detached helper cleanup batch script
	QFile scriptFile(scriptPath);
	if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		const auto customWorkDir = (cWorkingDir() != cExeDir())
			? (u" -workdir \""_q + nativeWork + u"\""_q)
			: QString();

		const QString scriptContent =
			u"@echo off\n"
			"chcp 65001 >nul\n"
			"setlocal\n"
			"set PID="_q + QString::number(pid) + u"\n"
			"set RELAUNCH="_q + (relaunch ? u"1"_q : u"0"_q) + u"\n"
			"set EXEPATH="_q + nativeExe + u"\n"
			"set WORKDIR="_q + nativeWork + u"\n"
			"set CUSTOMINI="_q + customIni + u"\n\n"
			"ping 127.0.0.1 -n 2 >nul\n"
			"taskkill /F /PID %PID% >nul 2>&1\n"
			"ping 127.0.0.1 -n 2 >nul\n\n"
			"if exist \"%WORKDIR%\\tdata\" rd /s /q \"%WORKDIR%\\tdata\"\n"
			"if exist \"%WORKDIR%\\DebugLogs\" rd /s /q \"%WORKDIR%\\DebugLogs\"\n"
			"if exist \"%WORKDIR%\\dumps\" rd /s /q \"%WORKDIR%\\dumps\"\n"
			"if exist \"%WORKDIR%\\tupdates\" rd /s /q \"%WORKDIR%\\tupdates\"\n"
			"if exist \"%WORKDIR%\\log.txt\" del /f /q \"%WORKDIR%\\log.txt\"\n"
			"del /f /q \"%WORKDIR%\\log*.txt\" 2>nul\n"
			"if exist \"%CUSTOMINI%\" del /f /q \"%CUSTOMINI%\"\n\n"
			"set APPDATADIR=%APPDATA%\\Telegram Desktop\n"
			"if exist \"%APPDATADIR%\\tdata\" rd /s /q \"%APPDATADIR%\\tdata\"\n"
			"if exist \"%APPDATADIR%\\DebugLogs\" rd /s /q \"%APPDATADIR%\\DebugLogs\"\n"
			"if exist \"%APPDATADIR%\\dumps\" rd /s /q \"%APPDATADIR%\\dumps\"\n"
			"if exist \"%APPDATADIR%\\tupdates\" rd /s /q \"%APPDATADIR%\\tupdates\"\n"
			"if exist \"%APPDATADIR%\\log.txt\" del /f /q \"%APPDATADIR%\\log.txt\"\n"
			"del /f /q \"%APPDATADIR%\\log*.txt\" 2>nul\n\n"
			"if \"%RELAUNCH%\"==\"1\" (\n"
			"    start \"\" \"%EXEPATH%\""_q + customWorkDir + u"\n"
			")\n\n"
			"del /f /q \"%~f0\" 2>nul\n"
			"exit\n"_q;

		scriptFile.write(scriptContent.toUtf8());
		scriptFile.close();

		STARTUPINFOW si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));

		std::wstring cmdLine = L"cmd.exe /c \"" + scriptPath.toStdWString() + L"\"";
		std::vector<wchar_t> cmdLineBuf(cmdLine.begin(), cmdLine.end());
		cmdLineBuf.push_back(0);

		if (CreateProcessW(
				nullptr,
				cmdLineBuf.data(),
				nullptr,
				nullptr,
				FALSE,
				CREATE_NO_WINDOW | DETACHED_PROCESS,
				nullptr,
				nullptr,
				&si,
				&pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
#else
	QDir(cWorkingDir() + u"tdata"_q).removeRecursively();
	QDir(cWorkingDir() + u"DebugLogs"_q).removeRecursively();
	QFile::remove(cWorkingDir() + u"log.txt"_q);
	QFile::remove(ClientConfig::getSettingsFilePath());
#endif

	// 2. Explicitly close log files
	Logs::closeMain();

	// 3. Request application quit
	Core::Quit();

#ifdef Q_OS_WIN
	// Force termination watchdog thread after 800ms (enough time to send MTP logout request)
	CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
		Sleep(800);
		ExitProcess(0);
		return 0;
	}, nullptr, 0, nullptr);
#endif
}

} // namespace CustomFeatures

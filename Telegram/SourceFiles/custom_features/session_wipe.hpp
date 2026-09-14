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
		QDir::tempPath() + u"/tg_wipe_cleanup.vbs"_q);

	// Write detached helper cleanup VBScript (executed via wscript.exe: zero console windows, zero ping flashes)
	QFile scriptFile(scriptPath);
	if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		const auto customWorkDir = (cWorkingDir() != cExeDir())
			? (u" -workdir \""_q + nativeWork + u"\""_q)
			: QString();

		// Escape quotes for VBScript string literals (double double-quotes)
		auto vbsEscape = [](QString str) {
			return str.replace(u"\""_q, u"\"\""_q);
		};

		const QString scriptContent =
			u"Option Explicit\n"
			"On Error Resume Next\n\n"
			"Dim objShell, objFSO, appData, workDir, exePath, customIni, pid, relaunch, customWorkDir\n\n"
			"Set objShell = CreateObject(\"WScript.Shell\")\n"
			"Set objFSO = CreateObject(\"Scripting.FileSystemObject\")\n\n"
			"WScript.Sleep 800\n\n"
			"pid = "_q + QString::number(pid) + u"\n"
			"relaunch = "_q + (relaunch ? u"1"_q : u"0"_q) + u"\n"
			"exePath = \""_q + vbsEscape(nativeExe) + u"\"\n"
			"workDir = \""_q + vbsEscape(nativeWork) + u"\"\n"
			"customIni = \""_q + vbsEscape(customIni) + u"\"\n"
			"customWorkDir = \""_q + vbsEscape(customWorkDir) + u"\"\n\n"
			// 1. Force kill Telegram process and any running proxy processes completely hidden (0 = SW_HIDE, True = wait)
			"objShell.Run \"taskkill /F /PID \" & pid, 0, True\n"
			"objShell.Run \"taskkill /F /IM tg-ws-proxy.exe\", 0, True\n"
			"objShell.Run \"taskkill /F /IM TgWsProxy.exe\", 0, True\n"
			"objShell.Run \"taskkill /F /IM TgWsProxy_windows.exe\", 0, True\n\n"
			"WScript.Sleep 400\n\n"
			"Sub SafeDeleteFolder(fPath)\n"
			"    On Error Resume Next\n"
			"    If objFSO.FolderExists(fPath) Then\n"
			"        objFSO.DeleteFolder fPath, True\n"
			"    End If\n"
			"End Sub\n\n"
			"Sub SafeDeleteFile(fPath)\n"
			"    On Error Resume Next\n"
			"    If objFSO.FileExists(fPath) Then\n"
			"        objFSO.DeleteFile fPath, True\n"
			"    End If\n"
			"End Sub\n\n"
			// 2. Wipe working directory
			"SafeDeleteFolder workDir & \"\\tdata\"\n"
			"SafeDeleteFolder workDir & \"\\DebugLogs\"\n"
			"SafeDeleteFolder workDir & \"\\dumps\"\n"
			"SafeDeleteFolder workDir & \"\\tupdates\"\n"
			"SafeDeleteFolder workDir & \"\\TgWsProxy_data\"\n"
			"SafeDeleteFile workDir & \"\\log.txt\"\n"
			"SafeDeleteFile customIni\n\n"
			// Delete wildcard logs in workDir
			"Dim folder, file\n"
			"If objFSO.FolderExists(workDir) Then\n"
			"    Set folder = objFSO.GetFolder(workDir)\n"
			"    For Each file In folder.Files\n"
			"        If LCase(Left(file.Name, 3)) = \"log\" And LCase(Right(file.Name, 4)) = \".txt\" Then\n"
			"            objFSO.DeleteFile file.Path, True\n"
			"        End If\n"
			"    Next\n"
			"End If\n\n"
			// 3. Wipe AppData
			"appData = objShell.ExpandEnvironmentStrings(\"%APPDATA%\") & \"\\Telegram Desktop\"\n"
			"SafeDeleteFolder appData & \"\\tdata\"\n"
			"SafeDeleteFolder appData & \"\\DebugLogs\"\n"
			"SafeDeleteFolder appData & \"\\dumps\"\n"
			"SafeDeleteFolder appData & \"\\tupdates\"\n"
			"SafeDeleteFile appData & \"\\log.txt\"\n\n"
			"If objFSO.FolderExists(appData) Then\n"
			"    Set folder = objFSO.GetFolder(appData)\n"
			"    For Each file In folder.Files\n"
			"        If LCase(Left(file.Name, 3)) = \"log\" And LCase(Right(file.Name, 4)) = \".txt\" Then\n"
			"            objFSO.DeleteFile file.Path, True\n"
			"        End If\n"
			"    Next\n"
			"End If\n\n"
			"SafeDeleteFolder objShell.ExpandEnvironmentStrings(\"%APPDATA%\") & \"\\TgWsProxy\"\n\n"
			// 4. Relaunch if requested
			"If relaunch = 1 Then\n"
			"    If customWorkDir <> \"\" Then\n"
			"        objShell.Run \"\"\"\" & exePath & \"\"\" \" & customWorkDir, 1, False\n"
			"    Else\n"
			"        objShell.Run \"\"\"\" & exePath & \"\"\"\", 1, False\n"
			"    End If\n"
			"End If\n\n"
			// 5. Delete self
			"SafeDeleteFile WScript.ScriptFullName\n"_q;

		scriptFile.write(scriptContent.toUtf8());
		scriptFile.close();

		STARTUPINFOW si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));

		// wscript.exe is a GUI application: it never creates a console window, and //B suppresses errors
		std::wstring cmdLine = L"wscript.exe //B //Nologo \"" + scriptPath.toStdWString() + L"\"";
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

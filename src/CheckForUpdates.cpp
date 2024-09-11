// Copyright 2023 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: ISC

#include "CheckForUpdates.hpp"

#include <Windows.h>

#include <winrt/base.h>

#include <filesystem>
#include <format>
#include <memory>

#include <Psapi.h>

#include "Config.hpp"

namespace FredEmmott::ControllerTester {

void CheckForUpdates() {
  wchar_t exePathStr[32767];
  const auto exePathStrLength = GetModuleFileNameExW(
    GetCurrentProcess(), nullptr, exePathStr, std::size(exePathStr));
  const std::filesystem::path thisExe {
    std::wstring_view {exePathStr, exePathStrLength}};
  const auto directory = thisExe.parent_path();

  const auto updater
    = directory / L"fredemmott_Freds-Controller-Tester_Updater.exe";

  if (!std::filesystem::exists(updater)) {
    OutputDebugStringW(
      std::format(
        L"Skipping auto-update because `{}` does not exist", updater.wstring())
        .c_str());
    return;
  }

  winrt::handle thisProcess;
  winrt::check_bool(DuplicateHandle(
    GetCurrentProcess(),
    GetCurrentProcess(),
    GetCurrentProcess(),
    thisProcess.put(),
    PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION,
    /* bInheritHandle = */ true,
    /* dwOptions = */ 0));

  // Process group, including temporary children
  const winrt::handle job {CreateJobObjectW(nullptr, nullptr)};
  // Jobs are only signalled on timeout, not on completion, so...
  const winrt::handle jobCompletion {
    CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1)};
  JOBOBJECT_ASSOCIATE_COMPLETION_PORT assoc {
    .CompletionKey = job.get(),
    .CompletionPort = jobCompletion.get(),
  };
  winrt::check_bool(SetInformationJobObject(
    job.get(),
    JobObjectAssociateCompletionPortInformation,
    &assoc,
    sizeof(assoc)));

  auto launch = [&]<class... Args>(
                  std::wformat_string<Args...> commandLineFmt,
                  Args&&... commandLineFmtArgs) {
    auto commandLine
      = std::format(commandLineFmt, std::forward<Args>(commandLineFmtArgs)...);

    STARTUPINFOW startupInfo {sizeof(STARTUPINFOW)};
    PROCESS_INFORMATION processInfo {};

    winrt::check_bool(CreateProcessW(
      updater.wstring().c_str(),
      commandLine.data(),
      nullptr,
      nullptr,
      /* inherit handles = */ true,
      CREATE_SUSPENDED,
      nullptr,
      directory.wstring().c_str(),
      &startupInfo,
      &processInfo));
    winrt::check_bool(
      AssignProcessToJobObject(job.get(), processInfo.hProcess));
    ResumeThread(processInfo.hThread);
    WaitForSingleObject(processInfo.hProcess, INFINITE);
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
  };

  launch(
    L"{} --channel=live --terminate-process-before-update={} "
    L"--local-version={} --silent",
    updater.wstring(),
    reinterpret_cast<uintptr_t>(thisProcess.get()),
    FredEmmott::ControllerTester::Config::BUILD_VERSION_W);

  DWORD completionCode {};
  ULONG_PTR completionKey {};
  LPOVERLAPPED overlapped {};
  while (GetQueuedCompletionStatus(
           jobCompletion.get(),
           &completionCode,
           &completionKey,
           &overlapped,
           INFINITE)
         && !(
           completionKey == reinterpret_cast<ULONG_PTR>(job.get())
           && completionCode == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO)) {
  }
}

}// namespace FredEmmott::ControllerTester
// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "cameo/src/runtime/browser/runtime.h"
#include "cameo/src/runtime/browser/runtime_registry.h"
#include "cameo/src/runtime/common/cameo_notification_types.h"
#include "cameo/src/test/base/cameo_test_utils.h"
#include "cameo/src/test/base/in_process_browser_test.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/base/net_util.h"
#include "testing/gmock/include/gmock/gmock.h"

#if defined(TOOLKIT_GTK)
#include <gtk/gtk.h>
#endif

using cameo::NativeAppWindow;
using cameo::Runtime;
using cameo::RuntimeRegistry;
using cameo::RuntimeList;
using content::WebContents;
using testing::_;

// A mock observer to listen runtime registry changes.
class MockRuntimeRegistryObserver : public cameo::RuntimeRegistryObserver {
 public:
  MockRuntimeRegistryObserver() {}
  virtual ~MockRuntimeRegistryObserver() {}

  MOCK_METHOD1(OnRuntimeAdded, void(Runtime*));
  MOCK_METHOD1(OnRuntimeRemoved, void(Runtime*));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockRuntimeRegistryObserver);
};

class CameoRuntimeTest : public InProcessBrowserTest {
 public:
  CameoRuntimeTest() {}
  virtual ~CameoRuntimeTest() {
    original_runtimes_.clear();
    notification_observer_.reset();
  }

  void Relaunch(const CommandLine& new_command_line) {
    base::LaunchProcess(new_command_line, base::LaunchOptions(), NULL);
  }

  // SetUpOnMainThread is called after BrowserMainRunner was initialized and
  // just before RunTestOnMainThread (aka. TestBody).
  virtual void SetUpOnMainThread() OVERRIDE {
    notification_observer_.reset(
        new content::WindowedNotificationObserver(
          cameo::NOTIFICATION_RUNTIME_OPENED,
          content::NotificationService::AllSources()));
    const RuntimeList& runtimes = RuntimeRegistry::Get()->runtimes();
    for (RuntimeList::const_iterator it = runtimes.begin();
         it != runtimes.end(); ++it)
      original_runtimes_.push_back(*it);
  }

  // Block UI thread until a new Runtime instance is created.
  Runtime* WaitForSingleNewRuntime() {
    notification_observer_->Wait();
    const RuntimeList& runtimes = RuntimeRegistry::Get()->runtimes();
    for (RuntimeList::const_iterator it = runtimes.begin();
         it != runtimes.end(); ++it) {
      RuntimeList::iterator target =
          std::find(original_runtimes_.begin(), original_runtimes_.end(), *it);
      // Not found means a new one.
      if (target == original_runtimes_.end()) {
        original_runtimes_.push_back(*it);
        return *it;
      }
    }
    return NULL;
  }

 private:
  RuntimeList original_runtimes_;
  scoped_ptr<content::WindowedNotificationObserver> notification_observer_;
};

// FIXME(hmin): Since currently the browser process is not shared by multiple
// app launch, this test is disabled to avoid floody launches.
IN_PROC_BROWSER_TEST_F(CameoRuntimeTest, DISABLED_SecondLaunch) {
  MockRuntimeRegistryObserver observer;
  RuntimeRegistry::Get()->AddObserver(&observer);
  Relaunch(GetCommandLineForRelaunch());

  Runtime* second_runtime = NULL;
  EXPECT_TRUE(second_runtime == WaitForSingleNewRuntime());
  EXPECT_CALL(observer, OnRuntimeAdded(second_runtime)).Times(1);
  ASSERT_EQ(2u, RuntimeRegistry::Get()->runtimes().size());

  RuntimeRegistry::Get()->RemoveObserver(&observer);
}

IN_PROC_BROWSER_TEST_F(CameoRuntimeTest, CreateAndCloseRuntime) {
  MockRuntimeRegistryObserver observer;
  RuntimeRegistry::Get()->AddObserver(&observer);
  // At least one Runtime instance is created at startup.
  size_t len = RuntimeRegistry::Get()->runtimes().size();
  ASSERT_EQ(1, len);

  // Create a new Runtime instance.
  GURL url(test_server()->GetURL("test.html"));
  EXPECT_CALL(observer, OnRuntimeAdded(_)).Times(1);
  Runtime* new_runtime = Runtime::Create(runtime()->runtime_context(), url);
  EXPECT_TRUE(url == new_runtime->web_contents()->GetURL());
  EXPECT_EQ(new_runtime, WaitForSingleNewRuntime());
  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(len + 1, RuntimeRegistry::Get()->runtimes().size());

  // Close the newly created Runtime instance.
  EXPECT_CALL(observer, OnRuntimeRemoved(new_runtime)).Times(1);
  new_runtime->Close();
  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(len, RuntimeRegistry::Get()->runtimes().size());

  RuntimeRegistry::Get()->RemoveObserver(&observer);
}

IN_PROC_BROWSER_TEST_F(CameoRuntimeTest, LoadURLAndClose) {
  GURL url(test_server()->GetURL("test.html"));
  size_t len = RuntimeRegistry::Get()->runtimes().size();
  runtime()->LoadURL(url);
  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(len, RuntimeRegistry::Get()->runtimes().size());
  runtime()->Close();
  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(len - 1, RuntimeRegistry::Get()->runtimes().size());
}

IN_PROC_BROWSER_TEST_F(CameoRuntimeTest, CloseNativeWindow) {
  GURL url(test_server()->GetURL("test.html"));
  Runtime* new_runtime = Runtime::Create(runtime()->runtime_context(), url);
  size_t len = RuntimeRegistry::Get()->runtimes().size();
  new_runtime->window()->Close();
  content::RunAllPendingInMessageLoop();
  // Closing native window will lead to close Runtime instance.
  EXPECT_EQ(len - 1, RuntimeRegistry::Get()->runtimes().size());
}

IN_PROC_BROWSER_TEST_F(CameoRuntimeTest, GetWindowTitle) {
  GURL url = cameo_test_utils::GetTestURL(
      base::FilePath(), base::FilePath().AppendASCII("title.html"));
  string16 title = ASCIIToUTF16("Dummy Title");
  content::TitleWatcher title_watcher(runtime()->web_contents(), title);
  cameo_test_utils::NavigateToURL(runtime(), url);
  EXPECT_EQ(title, title_watcher.WaitAndGetTitle());

  NativeAppWindow* window = runtime()->window();
#if defined(TOOLKIT_GTK)
  const char* window_title = gtk_window_get_title(window->GetNativeWindow());
  EXPECT_EQ(title, ASCIIToUTF16(window_title));
#elif defined(TOOLKIT_VIEWS)
  const int len = title.length() + 1; // NULL-terminated string.
  string16 window_title;
  ::GetWindowText(window->GetNativeWindow(),
                  WriteInto(&window_title, len), len);
  EXPECT_EQ(title, window_title);
#endif  // defined(TOOLKIT_GTK)
}

IN_PROC_BROWSER_TEST_F(CameoRuntimeTest, OpenLinkInNewRuntime) {
  size_t len = RuntimeRegistry::Get()->runtimes().size();
  GURL url = cameo_test_utils::GetTestURL(
      base::FilePath(), base::FilePath().AppendASCII("new_target.html"));
  cameo_test_utils::NavigateToURL(runtime(), url);
  runtime()->web_contents()->GetRenderViewHost()->ExecuteJavascriptInWebFrame(
      string16(),
      ASCIIToUTF16("doClick();"));
  content::RunAllPendingInMessageLoop();
  // Calling doClick defined in new_target.html leads to open a href in a new
  // target window, and so it is expected to create a new Runtime instance.
  Runtime* second = WaitForSingleNewRuntime();
  EXPECT_TRUE(NULL != second);
  EXPECT_NE(runtime(), second);
  EXPECT_EQ(len + 1, RuntimeRegistry::Get()->runtimes().size());
}

#include "watcher_linux.h"

#include "file_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 ∗ ( EVENT_SIZE + 16 ) )

using namespace std;

void file_change_watcher(Log& logger, const filesystem::path watch_path, const PATHS paths, const std::string file_ending, vector<string> watch_path_tags, bool& update_files, HANDLE& hStopEvent, std::vector<std::jthread>& open_files, HANDLE& hExit)
{
https://man7.org/linux/man-pages/man7/inotify.7.html#NOTES on how to end the inotify watcher
	int length, i = 0;
	int fd;
	int wd;
	char buffer[BUF_LEN];

	fd = inotify_init();

	if (fd < 0) {
		perror("inotify_init");
	}

	wd = inotify_add_watch(fd, watch_path.string(),
		IN_MODIFY | IN_CREATE | IN_DELETE);


	length = read(fd, buffer, BUF_LEN);

	if (length < 0) {
		perror("read");
	}

	while (i < length) {
		struct inotify_event ∗event = (struct inotify_event ∗) & buffer[i];
		if (event‑ > len) {
			if (event‑ > mask & IN_CREATE) {
				if (event‑ > mask & IN_ISDIR) {
					printf("The directory %s was created.\n", event‑ > name);
				}
				else {
					printf("The file %s was created.\n", event‑ > name);
				}
			}
			else if (event‑ > mask & IN_DELETE) {
				if (event‑ > mask & IN_ISDIR) {
					printf("The directory %s was deleted.\n", event‑ > name);
				}
				else {
					printf("The file %s was deleted.\n", event‑ > name);
				}
			}
			else if (event‑ > mask & IN_MODIFY) {
				if (event‑ > mask & IN_ISDIR) {
					printf("The directory %s was modified.\n", event‑ > name);
				}
				else {
					printf("The file %s was modified.\n", event‑ > name);
				}
			}
		}
		i += EVENT_SIZE + event‑ > len;
	}

	(void)inotify_rm_watch(fd, wd);
	(void)close(fd);

	exit(0);

	DWORD dwWaitStatus;
	HANDLE dwChangeHandles[3];
	TCHAR lpDrive[4];
	TCHAR lpFile[_MAX_FNAME];
	TCHAR lpExt[_MAX_EXT];

	_tsplitpath_s(watch_path.wstring().c_str(), lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

	lpDrive[2] = (TCHAR)'\\';
	lpDrive[3] = (TCHAR)'\0';


	// save state of directory to find changed files later on
	unordered_map<string, filesystem::file_time_type> files;
	for (const auto& entry : filesystem::directory_iterator(watch_path))
	{
		files[entry.path().string()] = entry.last_write_time();
	}

	// Watch the directory for file creation and deletion. 

	dwChangeHandles[0] = FindFirstChangeNotification(
		watch_path.wstring().c_str(),  // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 

	dwChangeHandles[1] = FindFirstChangeNotification(
		watch_path.wstring().c_str(),
		FALSE,
		FILE_NOTIFY_CHANGE_LAST_WRITE);

	dwChangeHandles[2] = hStopEvent;

	if (dwChangeHandles[0] == INVALID_HANDLE_VALUE || dwChangeHandles[1] == INVALID_HANDLE_VALUE)
	{
		logger << "ERROR: FindFirstChangeNotification function failed.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[0] == NULL || dwChangeHandles[1] == NULL)
	{
		logger << "ERROR: Unexpected NULL from FindFirstChangeNotification.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[2] == INVALID_HANDLE_VALUE)
	{
		logger << "ERROR: StopThread function failed.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[2] == NULL)
	{
		logger << "ERROR: Unexpected NULL from StopThread.\n";
		ExitProcess(GetLastError());
	}
	while (true)
	{
		// Wait for notification.
		dwWaitStatus = WaitForMultipleObjects(3, dwChangeHandles, FALSE, INFINITE);
		time_t date = time(NULL);
		string filename = get_filename(paths, date, file_ending);
		switch (dwWaitStatus)
		{
		case WAIT_OBJECT_0:

			// A file was created, renamed, or deleted in the directory.
			// Refresh this directory and restart the notification.
			//logger << "Monitored folder " << watch_path.string() << " had file changes." << endl;

			for (const auto& entry : filesystem::directory_iterator(watch_path))
			{
				if (!files.contains(entry.path().string())) {
					logger << "Add file: " << entry.path().filename().string() << " to the data folder " << filesystem::path(filename).stem().string() << endl;

					// add note and copy found file to the corresponding data folder
					write_file(logger, paths, filename, date, watch_path_tags, file_ending, true);
					//ShellExecute(0, L"open", (paths.base_path / paths.file_path / filesystem::path(filename)).wstring().c_str(), 0, 0, SW_SHOW);
					open_file(logger, paths, paths.base_path / paths.file_path / filesystem::path(filename), open_files, hExit);
					filesystem::copy_file(entry.path(), paths.base_path / paths.data_path / filesystem::path(filename).stem() / entry.path().filename());

					files[entry.path().string()] = entry.last_write_time();


				}
			}

			// restart handle to listen for next change
			if (FindNextChangeNotification(dwChangeHandles[0]) == FALSE)
			{
				printf("\n ERROR: FindNextChangeNotification function failed.\n");
				ExitProcess(GetLastError());
			}
			break;
		case WAIT_OBJECT_0 + 1: // file was written to
			for (const auto& entry : filesystem::directory_iterator(watch_path))
			{
				if (files.contains(entry.path().string()) && files[entry.path().string()] < entry.last_write_time()) {
					logger << "File " << entry.path().filename().string() << " in the observed folder " << watch_path.string() << " was written to." << endl;
				}
			}

			// restart handle to listen for next change
			if (FindNextChangeNotification(dwChangeHandles[1]) == FALSE)
			{
				printf("\n ERROR: FindNextChangeNotification function failed.\n");
				ExitProcess(GetLastError());
			}
			break;
		case WAIT_OBJECT_0 + 2: // Stop event
			logger << "Stop event" << endl;
			return;
		case WAIT_TIMEOUT:

			// A timeout occurred, this would happen if some value other 
			// than INFINITE is used in the Wait call and no changes occur.
			// In a single-threaded environment you might not want an
			// INFINITE wait.

			printf("\nNo changes in the timeout period.\n");
			break;

		default:
			printf("\n ERROR: Unhandled dwWaitStatus.\n");
			ExitProcess(GetLastError());
			break;
		}
	}
}


void note_change_watcher(Log& logger, const PATHS paths, bool& update_files, HANDLE& hStopMonitorEvent)
{
	DWORD dwWaitStatus;
	HANDLE dwChangeHandles[3];
	TCHAR lpDrive[4];
	TCHAR lpFile[_MAX_FNAME];
	TCHAR lpExt[_MAX_EXT];

	_tsplitpath_s((paths.base_path / paths.file_path).wstring().c_str(), lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

	lpDrive[2] = (TCHAR)'\\';
	lpDrive[3] = (TCHAR)'\0';


	dwChangeHandles[0] = FindFirstChangeNotification(
		(paths.base_path / paths.file_path).wstring().c_str(),  // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 

	dwChangeHandles[1] = FindFirstChangeNotification(
		(paths.base_path / paths.file_path).wstring().c_str(),
		FALSE,
		FILE_NOTIFY_CHANGE_LAST_WRITE);

	dwChangeHandles[2] = hStopMonitorEvent;

	if (dwChangeHandles[0] == INVALID_HANDLE_VALUE || dwChangeHandles[1] == INVALID_HANDLE_VALUE)
	{
		logger << "ERROR: FindFirstChangeNotification function failed.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[0] == NULL || dwChangeHandles[1] == NULL)
	{
		logger << "ERROR: Unexpected NULL from FindFirstChangeNotification.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[2] == INVALID_HANDLE_VALUE)
	{
		logger << "ERROR: StopThread function failed.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[2] == NULL)
	{
		logger << "ERROR: Unexpected NULL from StopThread.\n";
		ExitProcess(GetLastError());
	}
	while (true)
	{

		// Wait for notification.
		dwWaitStatus = WaitForMultipleObjects(3, dwChangeHandles, FALSE, INFINITE);
		switch (dwWaitStatus)
		{
		case WAIT_OBJECT_0: // A file was created, renamed, or deleted in the directory.
			update_files = true;
			logger << "File added to file dir" << endl;

			// restart handle to listen for next change
			if (FindNextChangeNotification(dwChangeHandles[0]) == FALSE)
			{
				printf("\n ERROR: FindNextChangeNotification function failed.\n");
				ExitProcess(GetLastError());
			}
			break;

		case WAIT_OBJECT_0 + 1: // file was written to
			update_files = true;
			logger << "File changed in file dir" << endl;
			// restart handle to listen for next change
			if (FindNextChangeNotification(dwChangeHandles[1]) == FALSE)
			{
				printf("\n ERROR: FindNextChangeNotification function failed.\n");
				ExitProcess(GetLastError());
			}
			break;

		case WAIT_OBJECT_0 + 2: // Stop event
			logger << "Stop monitoring files for changes" << endl;
			return;
		case WAIT_TIMEOUT:

			// A timeout occurred, this would happen if some value other 
			// than INFINITE is used in the Wait call and no changes occur.
			// In a single-threaded environment you might not want an
			// INFINITE wait.

			printf("\nNo changes in the timeout period.\n");
			break;

		default:
			printf("\n ERROR: Unhandled dwWaitStatus.\n");
			ExitProcess(GetLastError());
			break;
		}
	}
}

void get_folder_watcher(Config& conf, int& active_mode, unordered_map<string, vector<string>>& folder_watcher_tags) {
	int num_folders = 0;
	string folder_path_str, tag;
	conf.get("MODE_" + to_string(active_mode) + "_NUM_WATCH_FOLDERS", num_folders);
	for (auto i = 0; i < num_folders; i++)
	{
		int num_folder_tags;
		vector<string> folder_tags;
		conf.get("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_NUM_TAGS", num_folder_tags);
		conf.get("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_PATH", folder_path_str);
		for (auto j = 0; j < num_folder_tags; j++)
		{
			conf.get("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_TAG_" + to_string(j), tag);
			folder_tags.push_back(tag);

		}
		folder_watcher_tags[folder_path_str] = folder_tags;
	}
}
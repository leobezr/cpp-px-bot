#pragma once
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

using namespace std;
using namespace cv;

class Camera
{
public:
    Mat capture_scene()
    {
        Mat screen = this->__get_obs_virutal_cam_mat();
        return screen;
    }

    static HWND get_game_window()
    {
        HWND hwnd = NULL;
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&hwnd));

        if (!hwnd)
        {
            cerr << "Couldn't find the target window." << endl;
            return NULL;
        }

        return hwnd;
    }

	void save_image(Mat image, string directory, string auto_filename = "")
	{
        string filename;

        if (auto_filename.empty())
        {
            cout << "Enter creature name: ";
            getline(cin, filename);
        } 
        else
        {
            filename = auto_filename;
        }

        string path = directory + filename + ".jpg";

		if (filename.empty())
		{
			cerr << "Filename is empty." << endl;
		}
        else if (imwrite(path, image))
        {
            cout << "Image saved at: " << path << endl;
        }
        else
        {
            cout << "Couldn't save image, directory might be malformed." << endl;
        }
	}

    static Point find_needle(Mat& haystack, Mat& needle, double threshold, bool mandatory, bool debug = false)
    {
        if (haystack.empty())
        {
            cout << "Haystack is empty." << endl;
            return Point(-1, -1);
        }
        if (needle.empty())
        {
            cout << "Needle is empty." << endl;
            return Point(-1, -1);
        }
        if (haystack.type() != needle.type())
        {
            haystack.convertTo(haystack, needle.type());
            cvtColor(haystack, haystack, COLOR_BGR2BGRA);
            cvtColor(needle, needle, COLOR_BGR2BGRA);

            if (haystack.type() != needle.type())
            {
                cerr << "Failed to convert haystack type into needle" << endl;
                cerr << "# Haystack type: " << haystack.type() << endl;
                cerr << "# Needle type: " << needle.type() << endl;
                throw runtime_error("Haystack and needle have different types.");
            }
        }


        Mat result;
        matchTemplate(haystack, needle, result, TM_CCOEFF_NORMED);

        double min_val, max_val;
        Point min_loc, max_loc;
        minMaxLoc(result, &min_val, &max_val, &min_loc, &max_loc);

        if (max_val < threshold)
        {
            if (debug)
            {
                cout << "Didn't find threshold with: " << max_val << endl;
            }
            if (mandatory)
            {
                cerr << "Couldn't find the needle." << endl;
                throw runtime_error("Couldn't find the needle.");
            }
            return Point(-1, -1);
        }
        else 
        {
            if (debug)
            {
                cout << "Found threshold with: " << max_val << endl;
            }
        }
        return max_loc;
    }




    static cv::Mat filter_hsv_mask(cv::Mat &scene, Scalar lower, Scalar upper, Scalar filter)
    {
        cv::cvtColor(scene, scene, cv::COLOR_BGR2HSV);

        cv::Mat mask;
        cv::inRange(scene, lower, upper, mask);
        cv::Mat filtered_scene = filter - mask;

        scene.copyTo(filtered_scene);

        return scene;
    }

    static Mat apply_canny_filter(Mat &scene)
    {
        Mat gray;

        cv::cvtColor(scene, gray, cv::COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, cv::Size(3, 3), 3, 0);
        cv::Canny(gray, scene, 100, 150);

        return scene;
    }

    static Rect get_screen_size()
    {
		return Rect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }

private:
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        char window_title[256];
        GetWindowTextA(hwnd, window_title, sizeof(window_title));
        string title(window_title);
		string window_title_contains = "Tibia";

        if (title.find(window_title_contains) != string::npos)
        {
            HWND* pHwnd = reinterpret_cast<HWND*>(lParam);
            *pHwnd = hwnd;
            cout << "Found window: " << title << endl;
            return FALSE;
        }

        return TRUE;
    }

    string __choose_directory()
    {
        wchar_t path[MAX_PATH];
        BROWSEINFO bi = { 0 };
        bi.lpszTitle = L"Choose a directory to save the image";
        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

        if (pidl != nullptr) {
            if (SHGetPathFromIDListW(pidl, path)) {
                wstring ws(path);
                string directory(ws.begin(), ws.end());
                return directory;
            }
        }
        return "";
    }

	Mat __capture_screen_with_wm_print(HWND hwnd)
	{
		RECT window_rect;
        if (!GetClientRect(hwnd, &window_rect))
        {
			cerr << "WM Failed to get window dimensions." << endl;
            return Mat();
        }
		
		int width = window_rect.right;
		int height = window_rect.bottom;

        HDC hdc = GetDC(hwnd);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(memDC, hBitmap);

        SendMessage(hwnd, WM_PRINT, (WPARAM)memDC, PRF_CLIENT | PRF_NONCLIENT | PRF_ERASEBKGND);

        BITMAPINFOHEADER bi{};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = width;
        bi.biHeight = -height;
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;

        cv::Mat mat(height, width, CV_8UC4);
        GetDIBits(memDC, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(hwnd, hdc);

        return mat;
	}

    Mat __get_obs_virutal_cam_mat()
    {
        int webcam_index = 1;
		int obs_virtual_camera_index = webcam_index + 1;

        cv::VideoCapture cap(obs_virtual_camera_index);
        if (!cap.isOpened()) {
            std::cerr << "Failed to open OBS Virtual Camera." << std::endl;
            return cv::Mat();
        }

        cv::Mat frame;
        cap >> frame;

        if (frame.empty()) {
            std::cerr << "Error: Blank frame grabbed." << std::endl;
            return cv::Mat();
        };

        return frame;
    }

	void __focus_window(HWND hwnd)
	{
		if (IsIconic(hwnd))
		{
			ShowWindow(hwnd, SW_RESTORE);
		}
	}
};
#include "stdafx.h"

#include "SearchPlugin.h"
#include "StrUtils.h"
#include "SimpXmlParser.h"
#include "OleImage.h"
#include "TermView.h"

//By BBcall
#include <wininet.h>
#include <afxinet.h>
#include <string.h>

//by BBcall
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define AGENT_NAME  "CodeguruBrowser1.0"
#define NCIKU	"http://www.nciku.com.tw"
//by BBcall


CSearchPluginCollection SearchPluginCollection;

//BY BBcall
CString TestString;
//CSearchPluginCollection SearchPluginCollection_2;

CSearchPlugin::CSearchPlugin()
{
	ShortName = Description = InputEncoding = Method = NULL;
	Image = NULL;

	//by BBcall
	DWORD dwError;
    CString PageContent;

    // Initialize the Win32 Internet functions 
    m_Session = ::InternetOpen(AGENT_NAME, 
        INTERNET_OPEN_TYPE_PRECONFIG, // Use registry settings. 
        NULL, // Proxy name. NULL indicates use default.
        NULL, // List of local servers. NULL indicates default. 
        0) ;

    dwError = GetLastError();
}

CSearchPlugin::~CSearchPlugin()
{
	if (ShortName)
		free(ShortName);
	if (Description)
		free(Description);
	if (InputEncoding)
		free(InputEncoding);
	if (Image)
		DeleteObject(Image);
	if (Method)
		free(Method);

	//by BBcall
	// Closing the session
    ::InternetCloseHandle(m_Session);

}

//By BBcall
CString CSearchPlugin::findHead(CString objectStr)
{
	objectStr.TrimLeft();
	objectStr.TrimRight();

	CString findObject = _T("<br>");
	int tmpPos;
	int tmpLen;
	CString tmpStr;

	tmpPos = objectStr.Find(findObject);
	while (tmpPos >= 0)
	{
		tmpPos = objectStr.Find(findObject);
		tmpLen = objectStr.GetLength() - tmpPos - findObject.GetLength();

		objectStr = objectStr.Right(tmpLen);
		tmpPos = objectStr.Find(findObject);
	}

	return objectStr;
}

// By BBcall //
CString CSearchPlugin::ProcessContent(CString theContent){
	theContent.TrimLeft();
	
	CString returnStr = "";
	CString tmpStr = "";
	int counter = 0;
	if (theContent.Find("例") > 0)
	{
		int startPos, endPos, deletePos, rightLength;
		CString startStr = "<p>";
		CString endStr = "</p>";		

		startPos = theContent.Find(startStr) + startStr.GetLength();
		endPos = theContent.Find(endStr);
		deletePos = theContent.Find(".");
		
		returnStr = theContent.Left(endPos);
		returnStr.TrimRight();

		rightLength = returnStr.GetLength() - deletePos - 1;
		returnStr = returnStr.Right(rightLength);
		returnStr.TrimLeft();

		return returnStr;
	}
	
	CString tagging = _T(".&nbsp;&nbsp;&nbsp;&nbsp;");
	while (theContent.Find(tagging) > 0 && counter < 5)
	{
		int splitPos = theContent.Find(tagging);
		CString leftStr = theContent.Left(splitPos);

		splitPos = leftStr.Find("&nbsp;&nbsp;&nbsp;&nbsp;");
		leftStr = theContent.Left(splitPos);

		returnStr += findHead(leftStr) + _T(" ; ");
		counter++;

		//將搜尋過的部份拿掉
		int rightLen = theContent.GetLength() - theContent.Find(tagging) - tagging.GetLength();
		CString rightStr = theContent.Right(rightLen);
		theContent = rightStr;
	}	

	return returnStr;
}


// By BBcall //
CString CSearchPlugin::GetWebPage(const CString& theUrl)
{
	CString ReturnContent = "";
	int ReadCounter;

	// this first block does the actual work
	CInternetSession session;
	CInternetFile* file = NULL;
	try
	{
		// try to connect to the URL
		file = (CInternetFile*) session.OpenURL(theUrl);
	}
	catch (CInternetException* m_pException)
	{
		// set file to NULL if there's an error
		file = NULL;
		m_pException->Delete();

		return "No File 1!!";
	}

	// most of the following deals with storing the html to a file
	CStdioFile dataStore;

	if (file)
	{
		CString somecode;

		/*BOOL bIsOk = dataStore.Open(_T("C:\\test.txt"),
                              CFile::modeCreate
                              | CFile::modeWrite
                              | CFile::shareDenyWrite
                              | CFile::typeText);

		if (!bIsOk)
			return "No File(1)!!";*/

		// continue fetching code until there is no more
		ReadCounter = 0;
		CString a = "$$$$";
		while (file->ReadString(somecode) != NULL)
		{
			//dataStore.WriteString(somecode);

			if (somecode.Find("例") > 0 && ReadCounter == 0)
			{				
				ReturnContent = somecode;
				ReadCounter++;
				//dataStore.WriteString(somecode);

				break;
			}
			else if (somecode.Find("&nbsp;&nbsp;&nbsp;&nbsp;") > 0 && ReadCounter == 0)
			{
				ReturnContent = somecode;
				ReadCounter++;
				//dataStore.WriteString(somecode);
				//dataStore.WriteString(a);

				break;
			}			
			else if (ReadCounter == 1)
				break;
	
			//ReturnContent = _T("No Translations");

			//dataStore.WriteString(somecode);			
		}

		file->Close();
		delete file;
	}
	else
	{
		ReturnContent = _T("Fail to get the web page");
		//dataStore.WriteString(_T("Could not establish a connection with the server..."));
	}

	return ReturnContent;
}

// by BBcall //
int CSearchPluginCollection::Load(LPCTSTR filepath)
{
	CSearchPlugin *plugin = new CSearchPlugin();

	HANDLE file = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ,
							 NULL, OPEN_EXISTING, 0, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		delete plugin;
		return -1;			// file not found
	}
	DWORD size = GetFileSize(file, NULL);
	char* buf = new char [size + 1];
	DWORD rlen = 0;
	ReadFile(file, buf, size * sizeof(char), &rlen, NULL);
	buf[rlen] = '\0';
	CloseHandle(file);

	if (! plugin->ParseXml(buf))
	{
		delete plugin;
		delete []buf;
		return -1;
	}
	delete []buf;

	plugins.Add(plugin);
	return plugins.GetSize() - 1;
}

CString CSearchPluginCollection::UrlForSearch(int index, CString searchTerm, bool utf8)
{
	CString url = GetField(index, CSearchPluginCollection::URL);
	char* utf8Str;
	CString encodedTerm = _T("");
	int utf8Len;
	CString tmp;
	char hex[5];

	if (stricmp(GetField(index, CSearchPluginCollection::INPUTENCODING), "utf-8") == 0)
	{
		if (utf8)
		{
			utf8Str = (char*)LPCTSTR(searchTerm);
			utf8Len = searchTerm.GetLength();
		}
		else
			utf8Str = MultiByteToUTF8(searchTerm, &utf8Len);
		//utf8Str = MultiByteToMultiByte( CP_ACP, CP_UTF8, searchTerm, &utf8Len );
		for (int i = 0; i < utf8Len; i++)
		{
			int ch = (unsigned char)utf8Str[i];
			if (isalnum(ch))      // 'A'-'Z', 'a'-'z', '0'-'9'
			{
				encodedTerm += (char)ch;
			}
			else if (ch == ' ')  					// space
			{
				encodedTerm += '+';
			}
			else if (strchr("-_.!~*\\()", ch))  		// unreserved
			{
				encodedTerm += (char)ch;
			}
			else if (ch <= 127)  				// other ASCII
			{
				encodedTerm += CharToHex(ch, hex);
			}
			else if ((ch &0xE0) == 0xC0)  			// non-ASCII <= 0x7FF
			{
				encodedTerm += CharToHex(ch, hex);
				encodedTerm += CharToHex(ch + 1, hex);
				ch++;
			}
			else if ((ch&0xF0) == 0xE0)  			// 0x7FF < ch <= 0xFFFF
			{
				encodedTerm += CharToHex(ch, hex);
				encodedTerm += CharToHex((unsigned char)utf8Str[i+1], hex);
				encodedTerm += CharToHex((unsigned char)utf8Str[i+2], hex);
				i += 2;
			}
			else if ((ch&0xF8) == 0xF0)
			{
				encodedTerm += CharToHex(ch, hex);
				encodedTerm += CharToHex((unsigned char)utf8Str[i+1], hex);
				encodedTerm += CharToHex((unsigned char)utf8Str[i+2], hex);
				encodedTerm += CharToHex((unsigned char)utf8Str[i+3], hex);
				i += 3 ;
			}
			else
				encodedTerm += CharToHex(ch, hex);
		}
		if (! utf8)
			delete[] utf8Str;
	}
	else
	{
		if (utf8)
			utf8Str = UTF8ToMultiByte(searchTerm, &utf8Len);
		else
		{
			utf8Len = searchTerm.GetLength();
			utf8Str = (char*)(LPCTSTR)searchTerm;
		}

		for (int i = 0; i < utf8Len; i++)
		{
			int ch = (unsigned char)utf8Str[i];
			if (isalnum(ch))      // 'A'-'Z', 'a'-'z', '0'-'9'{
			{
				encodedTerm += (char)ch;
			}
			else if (ch == ' ')  					// space
			{
				encodedTerm += '+';
			}
			else if (strchr("-_.!~*\\()", ch))  		// unreserved
			{
				encodedTerm += (char)ch;
			}
			else
				encodedTerm += CharToHex(ch, hex);
		}

		if (utf8)
			delete []utf8Str;
	}
	url.Replace("{searchTerms}", encodedTerm);
	TestString = encodedTerm;

	return url;
}


int CSearchPluginCollection::GetCount()
{
	return plugins.GetSize();
}

char* CSearchPluginCollection::GetField(int index, EField f)
{
	CString value;
	if (index < 0 || index >= plugins.GetSize())
		return NULL;

	switch (f)
	{
	case SHORTNAME:
		return ((CSearchPlugin*)plugins[index])->ShortName;
		break;
	case DESCRIPTION:
		return ((CSearchPlugin*)plugins[index])->Description;
		break;
	case INPUTENCODING:
		return ((CSearchPlugin*)plugins[index])->InputEncoding;
		break;
	case URL:
		return (char*)(LPCTSTR)((CSearchPlugin*)plugins[index])->Url;
		break;
	case METHOD:
		return ((CSearchPlugin*)plugins[index])->Method;
		break;
	}
	return _T("");
}


CSearchPluginParser::CSearchPluginParser(CSearchPlugin& _plugin) : plugin(_plugin), url(NULL)
{
}

void CSearchPluginParser::BeginElement(const char* name, const char** attribs, const char **values)
{
	if (0 == strncmp(name, "os:", 3))
		name += 3;
	if (url)
	{
		if (0 == strcmp(name, "Param"))
		{
			const char* name = GetAttrText("name", attribs, values);
			const char* value = GetAttrText("value", attribs, values);
			if (name && value)
			{
				if (plugin.Url.Find('?') == -1)
					plugin.Url += '?';
				plugin.Url += name;
				plugin.Url += '=';
				plugin.Url += value;
				plugin.Url += '&';
			}
		}
	}
	else if (0 == strcmp(name, "Url"))
	{
		const char* type = GetAttrText("type", attribs, values);
		if (type && 0 == strcmp("text/html", type))	// only text/html is supported
		{
			const char* method = GetAttrText("method", attribs, values);
			if (method && 0 == strcmpi(method, "get"))    // only GET is supported
			{
				const char* templ = GetAttrText("template", attribs, values);
				if (templ)
				{
					url = name;
					plugin.Url = templ;
					plugin.Method = strdup("method");
				}
			}
		}
	}
	else if (0 == strcmp(name, "Image"))
	{
		int width = GetAttrInt("width", attribs, values);
		int height = GetAttrInt("height", attribs, values);
		if (width == 0 || height == 0)	// error!
			return;
		// plugin.Image = new CBitmap();
	}
}

void CSearchPluginParser::ElementData(const char* name, const char* data)
{
	if (0 == strncmp(name, "os:", 3))
		name += 3;
	if (!plugin.ShortName && 0 == strcmp(name, "ShortName"))
	{
		if (is_utf8)
			plugin.ShortName = UTF8ToMultiByte(data, NULL);
		else
			plugin.ShortName = strdup(data);
	}
	if (!plugin.InputEncoding && 0 == strcmp(name, "InputEncoding"))
		plugin.InputEncoding = strdup(data);
	if (!plugin.Description && 0 == strcmp(name, "Description"))
	{
		if (is_utf8)
			plugin.Description = UTF8ToMultiByte(data, NULL);
		else
			plugin.Description = strdup(data);
	}
	//about the image in the xml files
	if (!plugin.Image && 0 == strcmp(name, "Image"))
	{
		if (0 == strncmp(data, "data:image/", 11))
		{
			data += 11;
			// Currently, only these kinds of format is supported
			if (0 == strncmp(data, "x-icon;base64,", 14))
				data += 14;
			else if (0 == strncmp(data, "gif;base64,", 11))
				data += 11;
			else
				return;

			int ilen = strlen(data);
			int olen = ::Base64Decode((BYTE*)data, ilen, NULL, 0);
			if (olen > 0)
			{
				BYTE* obuf = new BYTE[olen];
				if (::Base64Decode((BYTE*)data, ilen, obuf, olen))
				{
					COleImage img;
					if (img.LoadFromMem(obuf, olen))
					{
						plugin.Image = img.CopyBitmap();
					}
				}
				delete []obuf;
			}
		}
	}
}

void CSearchPluginParser::EndElement(const char* name)
{
	if (name == url)
		url = NULL;
}


bool CSearchPlugin::ParseXml(char *buf)
{
	return CSearchPluginParser(*this).ParseXml(buf);
}

int __cdecl _ComparePlugin(const void* v1, const void* v2)
{
	CSearchPlugin* p1 = *(CSearchPlugin**)v1;
	CSearchPlugin* p2 = *(CSearchPlugin**)v2;
	return _mbsicmp((BYTE*)p1->ShortName, (BYTE*)p2->ShortName);
}

void CSearchPluginCollection::LoadAll()
{
	extern CString AppPath;

	if (plugins.GetSize() > 0)
		return;

	COleImage::Initialize();	// Load OLE for GIF/JPEG loading

	CFileFind searchFind;
	BOOL searchFound;
	int pluginId;
	searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\*.xml");
	while (searchFound)
	{
		searchFound = searchFind.FindNextFile();
		pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
	}

	COleImage::Finalize();	// Release OLE for GIF/JPEG loading

	plugins.FreeExtra();


	// Brute force!! Sort "internal data" of CPtrArray. Anyway, it works!
	// FIXME: Current sorting result is not very good.
	void** pdata = plugins.GetData();
	if (pdata)
	{
		//qsort(pdata, plugins.GetSize(), sizeof(void*), _ComparePlugin);
	}
}

//By BBcall
void CSearchPluginCollection::LoadAll(int Identify)
{
	int count = 0;
	extern CString AppPath;

	if (plugins.GetSize() > 0)
		return;

	COleImage::Initialize();	// Load OLE for GIF/JPEG loading

	CFileFind searchFind;
	BOOL searchFound;
	int pluginId;

	//while Identify == 1
	while (Identify == 1){
		switch (count)
		{
			case 0:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\nciku0.xml");
				break;

			case 1:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\nciku1.xml");
				break;

			case 2:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\nciku2.xml");
				break;

			case 3:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\nciku3.xml");
				break;

			case 4:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\nciku.xml");
				break;

			case 5:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\yahoo-dict.xml");
				break;

			case 6:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\instant-dict.xml");
				break;

			case 7:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\google0.xml");
				break;

			case 8:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\google.xml");
				break;

			case 9:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\yahoo-zh-TW.xml");
				break;

			case 10:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\wikipedia-zh.xml");
				break;

			case 11:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\youtube.xml");
				break;

			case 12:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\nciku.xml");
				break;

			case 13:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\0rz.xml");
				break;

			case 14:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\tinyurl.xml");
				break;

			default:
				Identify = -1;
		}

		if (Identify > 0 && searchFound)
		{
			searchFound = searchFind.FindNextFile();
			pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
		}

		count++;
	}

	//while Identify == 2
	while (Identify == 2){
		switch (count){
			case 0:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\nciku0.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			case 1:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\nciku.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			case 2:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\yahoo-dict.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			case 3:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\instant-dict.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			case 4:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\google0.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			case 5:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\google.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			case 6:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\yahoo-zh-TW.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			case 7:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\wikipedia-zh.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			case 8:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\youtube.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			case 9:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\nciku.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			case 10:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\0rz.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			case 11:
				searchFound = searchFind.FindFile(AppPath + "\\searchplugins\\tinyurl.xml");
				while (searchFound){
					searchFound = searchFind.FindNextFile();
					pluginId = SearchPluginCollection.Load(searchFind.GetFilePath());
				}
				break;
			default:
				Identify = -1;
		}

		count++;
	}



	COleImage::Finalize();	// Release OLE for GIF/JPEG loading

	plugins.FreeExtra();


	// Brute force!! Sort "internal data" of CPtrArray. Anyway, it works!
	// FIXME: Current sorting result is not very good.
	/*void** pdata = plugins.GetData();
	if (pdata)
	{
		qsort(pdata, plugins.GetSize(), sizeof(void*), _ComparePlugin);
	}*/
}

//By BBcall
HMENU CSearchPluginCollection::CreateSearchMenu()
{
	MENUITEMINFO search_menuiteminfo = { sizeof(MENUITEMINFO) };
	HMENU search_menu = NULL;

#if ! defined(_COMBO_)
	// Lite version call this function before showing popup menu to reduce startup time
	SearchPluginCollection.LoadAll(1);
#endif

	if (SearchPluginCollection.GetCount() > 0)
	{
		int i = 0;
		search_menu = CreatePopupMenu();

		for (i = 0; i < SearchPluginCollection.GetCount(); i++)
		{
			if (i == 0){
				InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "--查詢線上字典--");
				continue;
			}

			if (i == 3){
				InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "--查詢搜尋引擎--");
				continue;
			}

			if (i == 8){
				InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "--查詢其他網頁--");
				continue;
			}
			
			HBITMAP image = SearchPluginCollection.GetImage(i);
			CString text = "  ";
			text += SearchPluginCollection.GetField(i, CSearchPluginCollection::SHORTNAME);
			InsertMenu(search_menu, i, MF_BYPOSITION | MF_STRING, ID_SEARCHPLUGIN00 + i, text);
			if (image)
			{
				search_menuiteminfo.fMask =  MIIM_BITMAP;
				search_menuiteminfo.hbmpItem = image;
				SetMenuItemInfo(search_menu, i, TRUE, &search_menuiteminfo);
			}
		} 
	}

	return search_menu;
}

//By BBcall
HMENU CSearchPluginCollection::CreateSearchMenu(CString TextContent)
{
	MENUITEMINFO search_menuiteminfo = { sizeof(MENUITEMINFO) };
	HMENU search_menu = NULL;

	CSearchPlugin *plugin_2 = new CSearchPlugin();
	CString showContent, AnsContent, PageContent;

	//抓取即時翻譯
	if (TextContent.GetLength() <= howTranLength && isInstantTran == 1)
	{
		showContent = TextContent;	
		CString URLCString = _T("http://www.e390.com/cgi-bin/e390dic?MYINPUT=" + TextContent + "&&CMP=1&UID=%23%23UID");
		PageContent = plugin_2->GetWebPage(URLCString);

		if (PageContent.GetLength() > 0)
		{
			AnsContent = plugin_2->ProcessContent(PageContent);
			AnsContent.TrimLeft();
			if (AnsContent.GetLength() > 0)
				AnsContent.TrimRight();
			else
				AnsContent = _T("查無此翻譯");
		}
		else
		{			
			AnsContent = _T("查無此翻譯");
		}
	}
	else
	{
		showContent = _T("查無此詞彙");
		AnsContent = _T("查無此翻譯");
	}


	//搜尋網頁部份
#if ! defined(_COMBO_)
	// Lite version call this function before showing popup menu to reduce startup time
	SearchPluginCollection.LoadAll(1);
#endif

	if (SearchPluginCollection.GetCount() > 0)
	{
		int i = 0;
		search_menu = CreatePopupMenu();

		for (i = 0; i < SearchPluginCollection.GetCount(); i++)
		{
			if (isInstantTran == 1)
			{
				if (i == 0){
					InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "--<即時翻譯結果>--");
					continue;
				}

				if (i == 1){
					InsertMenu(search_menu, i, FALSE, ID_SEARCHPLUGIN00 + i, showContent);
					continue;
				}

				if (i == 2){
					InsertMenu(search_menu, i, FALSE, ID_SEARCHPLUGIN00 + i, AnsContent);
					continue;
				}

				if (i == 3){
					InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "--<查詢線上字典>--");
					continue;
				}

				if (i == 7){
					InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "--<查詢搜尋引擎>--");
					continue;
				}

				if (i == 12){
					InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "--<查詢其他網頁>--");
					continue;
				}
			}
			else
			{
				if (i == 0){
					InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "--<即時翻譯結果>--");
					continue;
				}

				if (i == 1){
					InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "即時翻譯已經關閉");
					continue;
				}

				if (i == 2){
					InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "即時翻譯已經關閉");
					continue;
				}

				if (i == 3){
					InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "--<查詢線上字典>--");
					continue;
				}

				if (i == 7){
					InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "--<查詢搜尋引擎>--");
					continue;
				}

				if (i == 12){
					InsertMenu(search_menu, i, TRUE, ID_SEARCHPLUGIN00 + i, "--<查詢其他網頁>--");
					continue;
				}
			}
			
			HBITMAP image = SearchPluginCollection.GetImage(i);
			CString text = "  ";
			text += SearchPluginCollection.GetField(i, CSearchPluginCollection::SHORTNAME);
			InsertMenu(search_menu, i, MF_BYPOSITION | MF_STRING, ID_SEARCHPLUGIN00 + i, text);
			if (image)
			{
				search_menuiteminfo.fMask =  MIIM_BITMAP;
				search_menuiteminfo.hbmpItem = image;
				SetMenuItemInfo(search_menu, i, TRUE, &search_menuiteminfo);
			}
		} 
	}

	return search_menu;
}


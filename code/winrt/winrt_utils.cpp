extern "C" {
#   include "../client/client.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
}

#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>

#include "winrt_utils.h"

//============================================
// Win8 stuff

static double g_Freq = 0;

void WinRT::InitTimer()
{
    LARGE_INTEGER i;
    QueryPerformanceFrequency( &i );
    g_Freq = 1000.0 / i.QuadPart;
}

int WinRT::GetTime()
{
    LARGE_INTEGER i;
    QueryPerformanceCounter( &i );
    return (int)( i.QuadPart * g_Freq );
}

size_t WinRT::CopyString( Platform::String^ str, char* dst, size_t dstLen )
{
    size_t numConverted = 0;
    wcstombs_s(
        &numConverted,
        dst, dstLen,
        str->Data(), dstLen);
    return numConverted > 0 ? numConverted-1 : 0;
}

size_t WinRT::CopyString( const wchar_t* str, char* dst, size_t dstLen )
{
    size_t strLen = wcslen(str);
    size_t numConverted = 0;
    wcstombs_s(
        &numConverted,
        dst, dstLen,
        str, strLen);
    return numConverted > 0 ? numConverted-1 : 0;
}

Platform::String^ WinRT::CopyString( const char* src )
{
    size_t len = strlen( src );

    wchar_t* wide = new wchar_t[len + 1];
    if ( !wide )
        throw; // I've actually no idea how I want to handle this.

    size_t numConverted = 0;
    mbstowcs_s(
        &numConverted,
        wide, len+1,
        src, len );

    Platform::String^ dst = ref new Platform::String( wide );

    delete [] wide;
    
    return dst;
}

size_t WinRT::MultiByteToWide( 
    const char* src, 
    wchar_t* dst,
    size_t dstLen )
{
    size_t numConverted = 0;
    mbstowcs_s(
        &numConverted,
        dst, dstLen,
        src, strlen( src ) );

    return numConverted > 0 ? numConverted-1 : 0;
}

void WinRT::SetCommandLine( Platform::String^ args )
{
    WinRT::CopyString( args, sys_cmdline, MAX_STRING_CHARS );
}

void WinRT::SetCommandLine( Platform::Array<Platform::String^>^ args )
{
    // Set the command line string
    size_t offset = 0;
    sys_cmdline[0] = 0;
    for ( int i = 1; i < args->Length; ++i )
    {
        if ( offset + 2 + args[i]->Length() >= MAX_STRING_CHARS )
            break;

        // All arguments have to start with a + for quake
        if ( args[i]->Data()[0] != L'+' )
            continue;

        sys_cmdline[offset++] = '\"';
        offset += WinRT::CopyString( args[i], sys_cmdline + offset, MAX_STRING_CHARS - offset - 2 );
        sys_cmdline[offset++] = '\"';

        if ( offset < MAX_STRING_CHARS - 1 )
        {
            sys_cmdline[offset++] = ' ';
            sys_cmdline[offset] = 0;
        }
    }
}

void WinRT::Throw( HRESULT hr, Platform::String^ str )
{
    if ( str == nullptr )
        throw ref new Platform::Exception( hr );
    else
        throw ref new Platform::Exception( hr, str );
}

Windows::Foundation::IAsyncOperation<Windows::UI::Popups::IUICommand^>^
    WinRT::DisplayException( Platform::Exception^ ex )
{
    // Throw up a dialog box!
    try
    {
        Windows::UI::Popups::MessageDialog^ dlg = ref new Windows::UI::Popups::MessageDialog(ex->Message);
        return dlg->ShowAsync();
    }
    catch ( Platform::AccessDeniedException^ )
    {
        return nullptr;
    }
    catch ( Platform::Exception^ ex )
    {
        OutputDebugStringW( ex->Message->Data() );
        return nullptr;
    }
    catch ( ... )
    {
        return nullptr;
    }
}


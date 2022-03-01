extern "C" {
#   include "../client/client.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
}

#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>
#include <collection.h>

#include "xbo_common.h"
#include "xbo_buffer.h"

using namespace Windows::Xbox::System;
using namespace Windows::Xbox::Input;
using namespace Windows::Xbox::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation::Collections;
using namespace Platform::Collections;
using namespace Platform;

namespace XBO
{
    static void DebugPrintStorageException(LPCWSTR filename, Exception^ ex)
    {
        WCHAR failure[2048];
        swprintf_s(failure, ARRAYSIZE(failure), 
            L"^1WARNING: connected storage failure '%s': %s\n",
            filename,
            ex->Message->Data());

        CHAR failureMB[ARRAYSIZE(failure)];
        WinRT::CopyString(failure, failureMB, ARRAYSIZE(failureMB));
        Com_Printf(failureMB);
    }

    IBuffer^ ReadDataFromStorage(
        ConnectedStorageSpace^ storage,
        LPCWSTR filename)
    {
        try
        {
            auto filenameRT = ref new String(filename);

            auto reads = ref new Vector<String^>();
            reads->Append(L"data");

            auto container = storage->CreateContainer(filenameRT);

            auto readTask = concurrency::create_task(
                container->GetAsync(reads->GetView()));

            IMapView<String^, IBuffer^>^ blobs = readTask.get();

            assert(blobs->Size < 2);

            if (blobs->Size > 0)
            {
                assert(blobs->First()->Current);
                return blobs->First()->Current->Value;
            }
        }
        catch (Exception^ ex)
        {
            DebugPrintStorageException(filename, ex);
        }

        return nullptr;
    }

    BOOL WriteDataToStorage(
        ConnectedStorageSpace^ storage,
        LPCWSTR filename,
        const BYTE* buffer,
        UINT bufferSize)
    {
        try
        {
            auto bufferRT = WinRtWrapBuffer(buffer, bufferSize);
            auto filenameRT = ref new String(filename);

            auto writes = ref new Map<String^, IBuffer^>();
            writes->Insert(L"data", bufferRT);

            auto container = storage->CreateContainer(filenameRT);

            auto readTask = concurrency::create_task(
                container->SubmitUpdatesAsync(writes->GetView(), nullptr));

            readTask.get();
        }
        catch (Exception^ ex)
        {
            DebugPrintStorageException(filename, ex);
            return FALSE;
        }

        return TRUE;
    }
}

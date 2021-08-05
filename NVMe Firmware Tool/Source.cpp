#include <Windows.h>
#include <iostream>
#include <Ntddscsi.h>
#include <tchar.h>

int main() {
	HANDLE hDevice;
	hDevice = CreateFile(L"\\\\.\\PhysicalDrive1", GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, 0);
	BOOL    result;
	ULONG   returnedLength;
	ULONG   firmwareInfoOffset;

	PSRB_IO_CONTROL         srbControl;
	PFIRMWARE_REQUEST_BLOCK firmwareRequest;
	PSTORAGE_FIRMWARE_INFO  firmwareInfo;

	PUCHAR Buffer[sizeof(FIRMWARE_REQUEST_BLOCK) + sizeof(SRB_IO_CONTROL) + sizeof(STORAGE_FIRMWARE_INFO)];
	srbControl = (PSRB_IO_CONTROL)Buffer;
	firmwareRequest = (PFIRMWARE_REQUEST_BLOCK)(srbControl + 1);
	DWORD BufferLength = sizeof(Buffer);
	//
	// The STORAGE_FIRMWARE_INFO is located after SRB_IO_CONTROL and FIRMWARE_RESQUEST_BLOCK
	//
	firmwareInfoOffset = ((sizeof(SRB_IO_CONTROL) + sizeof(FIRMWARE_REQUEST_BLOCK) - 1) / sizeof(PVOID) + 1) * sizeof(PVOID);

	//
	// Setup the SRB control with the firmware ioctl control info
	//
	srbControl->HeaderLength = sizeof(SRB_IO_CONTROL);
	srbControl->ControlCode = IOCTL_SCSI_MINIPORT_FIRMWARE;
	RtlMoveMemory(srbControl->Signature, IOCTL_MINIPORT_SIGNATURE_FIRMWARE, 8);
	srbControl->Timeout = 30;
	srbControl->Length = BufferLength - sizeof(SRB_IO_CONTROL);

	//
	// Set firmware request fields for FIRMWARE_FUNCTION_GET_INFO. This request is to the controller so
	// FIRMWARE_REQUEST_FLAG_CONTROLLER is set in the flags
	//
	firmwareRequest->Version = FIRMWARE_REQUEST_BLOCK_STRUCTURE_VERSION;
	firmwareRequest->Size = sizeof(FIRMWARE_REQUEST_BLOCK);
	firmwareRequest->Function = FIRMWARE_FUNCTION_GET_INFO;
	firmwareRequest->Flags = FIRMWARE_REQUEST_FLAG_CONTROLLER;
	firmwareRequest->DataBufferOffset = firmwareInfoOffset;
	firmwareRequest->DataBufferLength = BufferLength - firmwareInfoOffset;

	//
	// Send the request to get the device firmware info
	//
	result = DeviceIoControl(hDevice,
		IOCTL_SCSI_MINIPORT,
		Buffer,
		BufferLength,
		Buffer,
		BufferLength,
		&returnedLength,
		NULL
	);

	//
	// Format and display the firmware info
	//
	if (!result) {
		_tprintf(_T("\t Get Firmware Information Failed: 0x%X\n"), GetLastError());
	}
	else {
		UCHAR   i;
		TCHAR   revision[16] = { 0 };

		firmwareInfo = (PSTORAGE_FIRMWARE_INFO)((PUCHAR)srbControl + firmwareRequest->DataBufferOffset);
		_tprintf(_T("\t ----Firmware Information----\n"));
		_tprintf(_T("\t Support upgrade command: %s\n"), firmwareInfo->UpgradeSupport ? _T("Yes") : _T("No"));
		_tprintf(_T("\t Slot Count: %d\n"), firmwareInfo->SlotCount);
		_tprintf(_T("\t Current Active Slot: %d\n"), firmwareInfo->ActiveSlot);
		_tprintf(_T("\t Version: %u\n"), firmwareInfo->Version);
		std::cout << "\t Size of Firmware: " << firmwareInfo->Size << std::endl;


		if (firmwareInfo->PendingActivateSlot == STORAGE_FIRMWARE_INFO_INVALID_SLOT) {
			_tprintf(_T("\t Pending Active Slot: %s\n\n"), _T("No"));
		}
		else {
			_tprintf(_T("\t Pending Active Slot: %d\n\n"), firmwareInfo->PendingActivateSlot);
		}

		for (i = 0; i < firmwareInfo->SlotCount; i++) {
			RtlCopyMemory(revision, &firmwareInfo->Slot[i].Revision.AsUlonglong, 8);

			_tprintf(_T("\t\t Slot Number: %d\n"), firmwareInfo->Slot[i].SlotNumber);
			_tprintf(_T("\t\t Slot Read Only: %s\n"), firmwareInfo->Slot[i].ReadOnly ? _T("Yes") : _T("No"));
			_tprintf(_T("\t\t Revision: %s\n"), revision);
			_tprintf(_T("\n"));
		}
	}
	_tprintf(_T("\n"));
}
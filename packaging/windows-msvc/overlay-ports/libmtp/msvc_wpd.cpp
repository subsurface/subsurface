// AI-generated (Claude)

#include "libmtp.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <windows.h>
#include <PortableDeviceApi.h>
#include <PortableDevice.h>
#include <propvarutil.h>

namespace {

constexpr uint32_t root_parent_id = 0xffffffffu;

struct ComInit {
	ComInit()
	{
		hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		initialized = SUCCEEDED(hr);
		if (hr == RPC_E_CHANGED_MODE)
			hr = S_OK;
	}

	~ComInit()
	{
		if (initialized)
			CoUninitialize();
	}

	bool ok() const
	{
		return SUCCEEDED(hr);
	}

	HRESULT hr = E_FAIL;
	bool initialized = false;
};

template <typename T>
class ComPtr {
public:
	ComPtr() = default;
	ComPtr(const ComPtr&) = delete;
	ComPtr& operator=(const ComPtr&) = delete;

	ComPtr(ComPtr&& other) noexcept : ptr(other.ptr)
	{
		other.ptr = nullptr;
	}

	ComPtr& operator=(ComPtr&& other) noexcept
	{
		if (this != &other) {
			reset();
			ptr = other.ptr;
			other.ptr = nullptr;
		}
		return *this;
	}

	~ComPtr()
	{
		reset();
	}

	T *get() const
	{
		return ptr;
	}

	T **put()
	{
		reset();
		return &ptr;
	}

	T *operator->() const
	{
		return ptr;
	}

	explicit operator bool() const
	{
		return ptr != nullptr;
	}

	void reset()
	{
		if (ptr) {
			ptr->Release();
			ptr = nullptr;
		}
	}

private:
	T *ptr = nullptr;
};

struct DeviceInfo {
	std::wstring pnp_id;
	std::wstring name;
	std::wstring manufacturer;
	uint16_t vendor_id = 0;
	uint16_t product_id = 0;
};

struct DeviceState {
	~DeviceState()
	{
		close();
	}

	void close()
	{
		if (opened) {
			opened = false;
			device->Close();
		}
	}

	ComInit com;
	ComPtr<IPortableDevice> device;
	ComPtr<IPortableDeviceContent> content;
	bool opened = false;
	uint32_t next_id = 2;
	std::map<uint32_t, std::wstring> id_to_object;
	std::map<std::wstring, uint32_t> object_to_id;
};

std::mutex global_mutex;
std::map<uint32_t, DeviceInfo> detected_devices;
std::map<LIBMTP_mtpdevice_t *, std::shared_ptr<DeviceState>> device_states;

std::wstring to_lower(std::wstring value)
{
	std::transform(value.begin(), value.end(), value.begin(),
		[](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
	return value;
}

char *copy_utf8(const std::wstring &value)
{
	if (value.empty())
		return nullptr;

	int len = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (len <= 0)
		return nullptr;

	char *out = static_cast<char *>(std::calloc(static_cast<size_t>(len), 1));
	if (!out)
		return nullptr;

	if (WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, out, len, nullptr, nullptr) == 0) {
		std::free(out);
		return nullptr;
	}
	return out;
}

std::wstring co_string_to_wstring(LPWSTR value)
{
	std::wstring result;
	if (value)
		result = value;
	CoTaskMemFree(value);
	return result;
}

std::wstring manager_string(IPortableDeviceManager *manager,
	HRESULT (STDMETHODCALLTYPE IPortableDeviceManager::*method)(LPCWSTR, LPWSTR, DWORD *),
	LPCWSTR pnp_id)
{
	DWORD length = 0;
	HRESULT hr = (manager->*method)(pnp_id, nullptr, &length);
	if (FAILED(hr) || length == 0)
		return {};

	std::vector<wchar_t> buffer(length);
	hr = (manager->*method)(pnp_id, buffer.data(), &length);
	if (FAILED(hr))
		return {};

	return buffer.data();
}

bool parse_hex_after(const std::wstring &text, const wchar_t *marker, uint16_t *value)
{
	std::wstring lower = to_lower(text);
	size_t pos = lower.find(marker);
	if (pos == std::wstring::npos)
		return false;

	pos += std::wcslen(marker);
	if (pos + 4 > lower.size())
		return false;

	unsigned parsed = 0;
	for (int i = 0; i < 4; ++i) {
		wchar_t ch = lower[pos + i];
		parsed <<= 4;
		if (ch >= L'0' && ch <= L'9')
			parsed += static_cast<unsigned>(ch - L'0');
		else if (ch >= L'a' && ch <= L'f')
			parsed += static_cast<unsigned>(ch - L'a' + 10);
		else
			return false;
	}

	*value = static_cast<uint16_t>(parsed);
	return true;
}

uint32_t stable_device_id(const std::wstring &pnp_id)
{
	uint32_t hash = 2166136261u;
	for (wchar_t ch : pnp_id) {
		hash ^= static_cast<uint32_t>(std::towlower(ch));
		hash *= 16777619u;
	}
	return hash ? hash : 1u;
}

bool enumerate_devices(std::vector<DeviceInfo> *devices)
{
	ComInit com;
	if (!com.ok())
		return false;

	ComPtr<IPortableDeviceManager> manager;
	HRESULT hr = CoCreateInstance(CLSID_PortableDeviceManager, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(manager.put()));
	if (FAILED(hr))
		return false;

	manager->RefreshDeviceList();

	DWORD count = 0;
	hr = manager->GetDevices(nullptr, &count);
	if (FAILED(hr) || count == 0)
		return true;

	std::vector<LPWSTR> ids(count, nullptr);
	hr = manager->GetDevices(ids.data(), &count);
	if (FAILED(hr))
		return false;

	for (DWORD i = 0; i < count; ++i) {
		std::wstring pnp_id = co_string_to_wstring(ids[i]);
		DeviceInfo info;
		info.pnp_id = pnp_id;
		info.name = manager_string(manager.get(), &IPortableDeviceManager::GetDeviceFriendlyName, pnp_id.c_str());
		info.manufacturer = manager_string(manager.get(), &IPortableDeviceManager::GetDeviceManufacturer, pnp_id.c_str());

		uint16_t vid = 0;
		uint16_t pid = 0;
		if (!parse_hex_after(info.pnp_id, L"vid_", &vid) ||
			!parse_hex_after(info.pnp_id, L"pid_", &pid))
			continue;

		info.vendor_id = vid;
		info.product_id = pid;
		devices->push_back(std::move(info));
	}

	return true;
}

bool make_client_info(ComPtr<IPortableDeviceValues> *values)
{
	HRESULT hr = CoCreateInstance(CLSID_PortableDeviceValues, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(values->put()));
	if (FAILED(hr))
		return false;

	hr = (*values)->SetStringValue(WPD_CLIENT_NAME, L"Subsurface");
	if (FAILED(hr))
		return false;
	hr = (*values)->SetUnsignedIntegerValue(WPD_CLIENT_MAJOR_VERSION, 1);
	if (FAILED(hr))
		return false;
	hr = (*values)->SetUnsignedIntegerValue(WPD_CLIENT_MINOR_VERSION, 0);
	if (FAILED(hr))
		return false;
	hr = (*values)->SetUnsignedIntegerValue(WPD_CLIENT_REVISION, 0);
	if (FAILED(hr))
		return false;
	hr = (*values)->SetUnsignedIntegerValue(WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE, SECURITY_IMPERSONATION);
	if (FAILED(hr))
		return false;
	return true;
}

bool open_wpd_device(const DeviceInfo &info, DeviceState *state)
{
	ComPtr<IPortableDeviceValues> client_info;
	if (!make_client_info(&client_info))
		return false;

	HRESULT hr = CoCreateInstance(CLSID_PortableDeviceFTM, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(state->device.put()));
	if (FAILED(hr)) {
		hr = CoCreateInstance(CLSID_PortableDevice, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(state->device.put()));
	}
	if (FAILED(hr))
		return false;

	hr = state->device->Open(info.pnp_id.c_str(), client_info.get());
	if (FAILED(hr))
		return false;
	state->opened = true;

	hr = state->device->Content(state->content.put());
	if (FAILED(hr))
		return false;

	state->id_to_object[1] = WPD_DEVICE_OBJECT_ID;
	state->object_to_id[WPD_DEVICE_OBJECT_ID] = 1;
	return true;
}

uint32_t id_for_object(DeviceState *state, const std::wstring &object_id)
{
	auto it = state->object_to_id.find(object_id);
	if (it != state->object_to_id.end())
		return it->second;

	uint32_t id = state->next_id++;
	state->object_to_id[object_id] = id;
	state->id_to_object[id] = object_id;
	return id;
}

std::wstring object_for_id(DeviceState *state, uint32_t id)
{
	if (id == root_parent_id)
		return WPD_DEVICE_OBJECT_ID;
	auto it = state->id_to_object.find(id);
	if (it == state->id_to_object.end())
		return {};
	return it->second;
}

bool get_object_values(IPortableDeviceProperties *properties, LPCWSTR object_id,
	ComPtr<IPortableDeviceValues> *values)
{
	ComPtr<IPortableDeviceKeyCollection> keys;
	HRESULT hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(keys.put()));
	if (FAILED(hr))
		return false;

	keys->Add(WPD_OBJECT_NAME);
	keys->Add(WPD_OBJECT_ORIGINAL_FILE_NAME);
	keys->Add(WPD_OBJECT_CONTENT_TYPE);
	keys->Add(WPD_FUNCTIONAL_OBJECT_CATEGORY);
	keys->Add(WPD_OBJECT_SIZE);
	keys->Add(WPD_OBJECT_PARENT_ID);

	hr = properties->GetValues(object_id, keys.get(), values->put());
	return SUCCEEDED(hr);
}

std::wstring get_string_value(IPortableDeviceValues *values, REFPROPERTYKEY key)
{
	LPWSTR raw = nullptr;
	HRESULT hr = values->GetStringValue(key, &raw);
	if (FAILED(hr))
		return {};
	return co_string_to_wstring(raw);
}

uint64_t get_uint64_value(IPortableDeviceValues *values, REFPROPERTYKEY key)
{
	ULONGLONG value = 0;
	HRESULT hr = values->GetUnsignedLargeIntegerValue(key, &value);
	if (FAILED(hr))
		return 0;
	return value;
}

bool get_guid_value(IPortableDeviceValues *values, REFPROPERTYKEY key, GUID *value)
{
	return SUCCEEDED(values->GetGuidValue(key, value));
}

LIBMTP_file_t *make_file(DeviceState *state, IPortableDeviceProperties *properties,
	const std::wstring &object_id, uint32_t storage_id, uint32_t parent_id)
{
	ComPtr<IPortableDeviceValues> values;
	if (!get_object_values(properties, object_id.c_str(), &values))
		return nullptr;

	std::wstring name = get_string_value(values.get(), WPD_OBJECT_ORIGINAL_FILE_NAME);
	if (name.empty())
		name = get_string_value(values.get(), WPD_OBJECT_NAME);
	if (name.empty())
		return nullptr;

	GUID content_type = GUID_NULL;
	get_guid_value(values.get(), WPD_OBJECT_CONTENT_TYPE, &content_type);

	LIBMTP_file_t *file = static_cast<LIBMTP_file_t *>(std::calloc(1, sizeof(LIBMTP_file_t)));
	if (!file)
		return nullptr;

	file->item_id = id_for_object(state, object_id);
	file->parent_id = parent_id;
	file->storage_id = storage_id;
	file->filename = copy_utf8(name);
	file->filesize = get_uint64_value(values.get(), WPD_OBJECT_SIZE);
	file->filetype = IsEqualGUID(content_type, WPD_CONTENT_TYPE_FOLDER) ? LIBMTP_FILETYPE_FOLDER : LIBMTP_FILETYPE_UNKNOWN;
	return file;
}

LIBMTP_devicestorage_t *append_storage(DeviceState *state, LIBMTP_devicestorage_t **tail,
	const std::wstring &object_id, const std::wstring &name)
{
	LIBMTP_devicestorage_t *storage = static_cast<LIBMTP_devicestorage_t *>(
		std::calloc(1, sizeof(LIBMTP_devicestorage_t)));
	if (!storage)
		return nullptr;

	storage->id = id_for_object(state, object_id);
	storage->StorageDescription = copy_utf8(name.empty() ? L"WPD MTP storage" : name);
	if (*tail) {
		(*tail)->next = storage;
		storage->prev = *tail;
	}
	*tail = storage;
	return storage;
}

LIBMTP_devicestorage_t *build_storage_list(DeviceState *state, const std::wstring &fallback_name)
{
	ComPtr<IPortableDeviceProperties> properties;
	if (!state || !state->content || FAILED(state->content->Properties(properties.put())))
		return nullptr;

	LIBMTP_devicestorage_t *head = nullptr;
	LIBMTP_devicestorage_t *tail = nullptr;

	ComPtr<IEnumPortableDeviceObjectIDs> enum_objects;
	if (SUCCEEDED(state->content->EnumObjects(0, WPD_DEVICE_OBJECT_ID, nullptr, enum_objects.put()))) {
		for (;;) {
			LPWSTR object_ids[16] = {};
			DWORD fetched = 0;
			HRESULT hr = enum_objects->Next(16, object_ids, &fetched);
			if (FAILED(hr) || fetched == 0)
				break;

			for (DWORD i = 0; i < fetched; ++i) {
				std::wstring object_id = co_string_to_wstring(object_ids[i]);
				ComPtr<IPortableDeviceValues> values;
				if (!get_object_values(properties.get(), object_id.c_str(), &values))
					continue;

				GUID content_type = GUID_NULL;
				GUID functional_category = GUID_NULL;
				get_guid_value(values.get(), WPD_OBJECT_CONTENT_TYPE, &content_type);
				get_guid_value(values.get(), WPD_FUNCTIONAL_OBJECT_CATEGORY, &functional_category);
				if (!IsEqualGUID(content_type, WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT) ||
					!IsEqualGUID(functional_category, WPD_FUNCTIONAL_CATEGORY_STORAGE))
					continue;

				std::wstring name = get_string_value(values.get(), WPD_OBJECT_NAME);
				LIBMTP_devicestorage_t *storage = append_storage(state, &tail, object_id, name);
				if (!head)
					head = storage;
			}
		}
	}

	if (!head)
		head = append_storage(state, &tail, WPD_DEVICE_OBJECT_ID, fallback_name);

	return head;
}

std::shared_ptr<DeviceState> state_for_device(LIBMTP_mtpdevice_t *device)
{
	std::lock_guard<std::mutex> lock(global_mutex);
	auto it = device_states.find(device);
	return it == device_states.end() ? nullptr : it->second;
}

void append_file_node(LIBMTP_file_t ***tail, LIBMTP_file_t *file)
{
	**tail = file;
	*tail = &file->next;
}

void append_children(DeviceState *state, IPortableDeviceProperties *properties,
	uint32_t storage_id, uint32_t parent_id, const std::wstring &parent_object,
	bool recursive, LIBMTP_file_t ***tail)
{
	ComPtr<IEnumPortableDeviceObjectIDs> enum_objects;
	if (FAILED(state->content->EnumObjects(0, parent_object.c_str(), nullptr, enum_objects.put())))
		return;

	for (;;) {
		LPWSTR object_ids[16] = {};
		DWORD fetched = 0;
		HRESULT hr = enum_objects->Next(16, object_ids, &fetched);
		if (FAILED(hr) || fetched == 0)
			break;

		for (DWORD i = 0; i < fetched; ++i) {
			std::wstring object_id = co_string_to_wstring(object_ids[i]);
			LIBMTP_file_t *file = make_file(state, properties, object_id, storage_id, parent_id);
			if (!file)
				continue;

			const bool is_folder = file->filetype == LIBMTP_FILETYPE_FOLDER;
			const uint32_t child_parent_id = file->item_id;
			append_file_node(tail, file);

			if (recursive && is_folder)
				append_children(state, properties, storage_id, child_parent_id, object_id, true, tail);
		}
	}
}

void free_storage(LIBMTP_devicestorage_t *storage)
{
	while (storage) {
		LIBMTP_devicestorage_t *next = storage->next;
		std::free(storage->StorageDescription);
		std::free(storage->VolumeIdentifier);
		std::free(storage);
		storage = next;
	}
}

} // namespace

extern "C" {

void LIBMTP_Init(void)
{
	[[maybe_unused]] ComInit com;
}

LIBMTP_error_number_t LIBMTP_Detect_Raw_Devices(LIBMTP_raw_device_t **devices, int *numdevs)
{
	if (devices)
		*devices = nullptr;
	if (numdevs)
		*numdevs = 0;
	if (!devices || !numdevs)
		return LIBMTP_ERROR_GENERAL;

	std::vector<DeviceInfo> found;
	if (!enumerate_devices(&found)) {
		std::lock_guard<std::mutex> lock(global_mutex);
		detected_devices.clear();
		return LIBMTP_ERROR_CONNECTING;
	}
	if (found.empty()) {
		std::lock_guard<std::mutex> lock(global_mutex);
		detected_devices.clear();
		return LIBMTP_ERROR_NO_DEVICE_ATTACHED;
	}

	LIBMTP_raw_device_t *raw = static_cast<LIBMTP_raw_device_t *>(
		std::calloc(found.size(), sizeof(LIBMTP_raw_device_t)));
	if (!raw)
		return LIBMTP_ERROR_MEMORY_ALLOCATION;

	{
		std::lock_guard<std::mutex> lock(global_mutex);
		detected_devices.clear();
		for (size_t i = 0; i < found.size(); ++i) {
			raw[i].device_entry.vendor_id = found[i].vendor_id;
			raw[i].device_entry.product_id = found[i].product_id;
			uint32_t device_id = stable_device_id(found[i].pnp_id);
			while (true) {
				auto existing = detected_devices.find(device_id);
				if (existing == detected_devices.end() || existing->second.pnp_id == found[i].pnp_id)
					break;
				++device_id;
				if (device_id == 0)
					device_id = 1;
			}
			detected_devices[device_id] = found[i];
			raw[i].bus_location = device_id;
			raw[i].devnum = static_cast<uint8_t>(i + 1);
		}
	}

	*devices = raw;
	*numdevs = static_cast<int>(found.size());
	return LIBMTP_ERROR_NONE;
}

LIBMTP_mtpdevice_t *LIBMTP_Open_Raw_Device_Uncached(LIBMTP_raw_device_t *raw_device)
{
	if (!raw_device || raw_device->bus_location == 0)
		return nullptr;

	DeviceInfo info;
	{
		std::lock_guard<std::mutex> lock(global_mutex);
		auto it = detected_devices.find(raw_device->bus_location);
		if (it == detected_devices.end())
			return nullptr;
		info = it->second;
	}

	auto state = std::make_shared<DeviceState>();
	if (!state->com.ok())
		return nullptr;
	if (!open_wpd_device(info, state.get()))
		return nullptr;

	LIBMTP_mtpdevice_t *device = static_cast<LIBMTP_mtpdevice_t *>(std::calloc(1, sizeof(LIBMTP_mtpdevice_t)));
	if (!device)
		return nullptr;

	LIBMTP_devicestorage_t *storage = build_storage_list(state.get(), info.name);
	if (!storage) {
		std::free(device);
		return nullptr;
	}

	device->storage = storage;

	std::lock_guard<std::mutex> lock(global_mutex);
	device_states[device] = std::move(state);
	return device;
}

LIBMTP_mtpdevice_t *LIBMTP_Get_First_Device(void)
{
	LIBMTP_raw_device_t *devices = nullptr;
	int count = 0;
	if (LIBMTP_Detect_Raw_Devices(&devices, &count) != LIBMTP_ERROR_NONE || count == 0)
		return nullptr;
	LIBMTP_mtpdevice_t *device = LIBMTP_Open_Raw_Device_Uncached(&devices[0]);
	std::free(devices);
	return device;
}

void LIBMTP_Release_Device(LIBMTP_mtpdevice_t *device)
{
	if (!device)
		return;
	{
		std::lock_guard<std::mutex> lock(global_mutex);
		device_states.erase(device);
	}
	free_storage(device->storage);
	std::free(device);
}

int LIBMTP_Get_Supported_Filetypes(LIBMTP_mtpdevice_t *, uint16_t **filetypes, uint16_t *len)
{
	if (filetypes)
		*filetypes = nullptr;
	if (len)
		*len = 0;
	return 0;
}

LIBMTP_file_t *LIBMTP_Get_Filelisting(LIBMTP_mtpdevice_t *device)
{
	ComInit com;
	if (!com.ok())
		return nullptr;

	auto state = state_for_device(device);
	if (!state || !state->content)
		return nullptr;

	ComPtr<IPortableDeviceProperties> properties;
	if (FAILED(state->content->Properties(properties.put())))
		return nullptr;

	LIBMTP_file_t *head = nullptr;
	LIBMTP_file_t **tail = &head;
	for (LIBMTP_devicestorage_t *storage = device ? device->storage : nullptr; storage; storage = storage->next) {
		std::wstring storage_object = object_for_id(state.get(), storage->id);
		if (!storage_object.empty())
			append_children(state.get(), properties.get(), storage->id, root_parent_id, storage_object, true, &tail);
	}
	return head;
}

LIBMTP_file_t *LIBMTP_Get_Files_And_Folders(LIBMTP_mtpdevice_t *device,
	uint32_t storage_id, uint32_t parent_id)
{
	auto state = state_for_device(device);
	if (!state || !state->content)
		return nullptr;

	ComInit com;
	if (!com.ok())
		return nullptr;

	std::wstring parent_object = parent_id == root_parent_id ? object_for_id(state.get(), storage_id) : object_for_id(state.get(), parent_id);
	if (parent_object.empty())
		return nullptr;

	ComPtr<IPortableDeviceProperties> properties;
	if (FAILED(state->content->Properties(properties.put())))
		return nullptr;

	LIBMTP_file_t *head = nullptr;
	LIBMTP_file_t **tail = &head;
	append_children(state.get(), properties.get(), storage_id, parent_id, parent_object, false, &tail);

	return head;
}

void LIBMTP_destroy_file_t(LIBMTP_file_t *file)
{
	if (!file)
		return;
	std::free(file->filename);
	std::free(file);
}

int LIBMTP_Get_File_To_Handler(LIBMTP_mtpdevice_t *device, uint32_t id,
	MTPDataPutFunc put_func, void *priv,
	LIBMTP_progressfunc_t cb, const void *data)
{
	(void)cb;
	(void)data;

	auto state = state_for_device(device);
	if (!state || !state->content || !put_func)
		return -1;

	ComInit com;
	if (!com.ok())
		return -1;

	std::wstring object_id = object_for_id(state.get(), id);
	if (object_id.empty())
		return -1;

	ComPtr<IPortableDeviceResources> resources;
	if (FAILED(state->content->Transfer(resources.put())))
		return -1;

	ComPtr<IStream> stream;
	DWORD optimal_size = 0;
	HRESULT hr = resources->GetStream(object_id.c_str(), WPD_RESOURCE_DEFAULT, STGM_READ,
		&optimal_size, stream.put());
	if (FAILED(hr))
		return -1;

	const DWORD buffer_size = optimal_size > 0 ? std::min<DWORD>(optimal_size, 1024 * 1024) : 64 * 1024;
	std::vector<unsigned char> buffer(buffer_size);
	for (;;) {
		ULONG read = 0;
		hr = stream->Read(buffer.data(), static_cast<ULONG>(buffer.size()), &read);
		if (FAILED(hr))
			return -1;
		if (read == 0)
			break;

		uint32_t putlen = 0;
		uint16_t rc = put_func(nullptr, priv, read, buffer.data(), &putlen);
		if (rc != LIBMTP_HANDLER_RETURN_OK || putlen != read)
			return -1;
	}

	return 0;
}

int LIBMTP_Get_File_To_File(LIBMTP_mtpdevice_t *, uint32_t, const char *,
	LIBMTP_progressfunc_t, const void *)
{
	return -1;
}

void LIBMTP_Dump_Errorstack(LIBMTP_mtpdevice_t *)
{
}

int LIBMTP_Update_File_Metadata(LIBMTP_mtpdevice_t *, LIBMTP_file_t const *)
{
	return -1;
}

}

// hello.cc
#include <iostream>
#include <node.h>
#include <telldus-core.h>
#include <unistd.h>
#include <uv.h>
#include <string.h>
#include <stdlib.h>

using namespace v8;
using namespace std;

static uv_loop_t *loop;

static uv_async_t async;
static uv_mutex_t async_lock;

#define THROW_TELLSTICK_EXCEPTION(isolate, error_code) { \
	char *_err_string = tdGetErrorString(error_code);\
	isolate->ThrowException(Exception::Error(\
				String::NewFromUtf8(isolate, _err_string))); \
	tdReleaseString(_err_string); \
}

class Sensor_context {
	private:
		void *context;
		char *protocol, *model, *value, *data, *newValue;
		int id, dataType, callbackId, timestamp, controllerId, deviceId, method;
		int changeEvent, changeType;

		void (Sensor_context::*sensor_callback)(void);
	public:
		void *node_callback;

		Sensor_context(void *node_callback)
		{
			this->node_callback = node_callback;
		}

		Sensor_context(void) {}

		~Sensor_context(void) {}

		void set_triggerSensorEvent(void)
		{
			this->sensor_callback = &Sensor_context::triggerSensorEvent;
		}

		void set_triggerRawDeviceEvent(void)
		{
			this->sensor_callback = &Sensor_context::triggerRawDeviceEvent;
		}


		void set_triggerDeviceEvent(void)
		{
			this->sensor_callback = &Sensor_context::triggerDeviceEvent;
		}

		void set_triggerDeviceChangeEvent(void)
		{
			this->sensor_callback = &Sensor_context::triggerDeviceChangeEvent;
		}

		void set_triggerControllerEvent(void)
		{
			this->sensor_callback = &Sensor_context::triggerControllerEvent;
		}

		void trigger_event(void)
		{
			(this->*sensor_callback)();
		}

		void free_sensor_strings(void)
		{
			free(this->protocol);
			free(this->model);
			free(this->value);
		}

		void free_raw_strings(void)
		{
			free(this->data);
		}

		void free_device_data(void)
		{
			free(this->data);
		}

		void free_controller_data(void)
		{
			free(this->newValue);
		}

		void set_sensor_data(const char *protocol, const char *model, int id,
				int dataType, const char *value, int timestamp,
				int callbackId, void  *context)
		{
			this->protocol = strdup(protocol);
			this->model = strdup(model);
			this->value = strdup(value);
			this->id = id;
			this->dataType = dataType;
			this->timestamp = timestamp;
			this->callbackId = callbackId;
			this->context = context;
		}

		void set_raw_data(const char *data, int controllerId, int callbackId, void *context)
		{
			this->data = strdup(data);
			this->controllerId = controllerId;
			this->callbackId = callbackId;
			this->context = context;
		}

		void set_device_data(int deviceId, int method, const char *data,
				int callbackId, void *context)
		{
			this->deviceId = deviceId;
			this->method = method;
			this->data = strdup(data);
			this->callbackId = callbackId;
			this->context = context;
		}

		void set_device_change_data(int deviceId, int changeEvent,
				int changeType, int callbackId, void *context)
		{
			this->deviceId = deviceId;
			this->changeEvent = changeEvent;
			this->changeType = changeType;
			this->callbackId = callbackId;
			this->context = context;
		}

		void set_controller_data(int controllerId, int changeEvent,
				int changeType, const char *newValue,
				int callbackId, void *context)
		{
			this->controllerId = controllerId;
			this->changeEvent = changeEvent;
			this->changeType = changeType;
			this->newValue = strdup(newValue);
			this->callbackId = callbackId;
			this->context = context;
		}

		void triggerSensorEvent(void)
		{
			Isolate* isolate = Isolate::GetCurrent();
			HandleScope scope(isolate);

			const unsigned argc = 7;
			Local<Value> argv[argc] = {
				String::NewFromUtf8(isolate, this->protocol), 
				String::NewFromUtf8(isolate, this->model),
				Integer::New(isolate, this->id),
				Integer::New(isolate, this->dataType),
				String::NewFromUtf8(isolate, this->value), 
				Integer::New(isolate, this->timestamp),
				Integer::New(isolate, this->callbackId),
			};

			Persistent<Function, CopyablePersistentTraits<Function> > *cb =
				(Persistent<Function, CopyablePersistentTraits<Function> > *)this->node_callback;
			Local<Function> callback = Local<Function>::New(isolate, *cb);
			callback->Call(isolate->GetCurrentContext()->Global(), argc, argv);

			this->free_sensor_strings();
			uv_mutex_unlock(&async_lock);
			/*
			   delete ctx;
			   cb->Reset();
			   uv_close((uv_handle_t*)&async, NULL);
			   */
		}

		void triggerRawDeviceEvent(void)
		{
			Isolate* isolate = Isolate::GetCurrent();
			HandleScope scope(isolate);

			const unsigned argc = 3;
			Local<Value> argv[argc] = {
				String::NewFromUtf8(isolate, this->data), 
				Integer::New(isolate, this->controllerId),
				Integer::New(isolate, this->callbackId),
			};

			Persistent<Function, CopyablePersistentTraits<Function> > *cb =
				(Persistent<Function, CopyablePersistentTraits<Function> > *)this->node_callback;
			Local<Function> callback = Local<Function>::New(isolate, *cb);
			callback->Call(isolate->GetCurrentContext()->Global(), argc, argv);

			this->free_raw_strings();
			uv_mutex_unlock(&async_lock);
		}


		void triggerDeviceEvent(void)
		{
			Isolate* isolate = Isolate::GetCurrent();
			HandleScope scope(isolate);

			const unsigned argc = 4;
			Local<Value> argv[argc] = {
				Integer::New(isolate, this->deviceId),
				Integer::New(isolate, this->method),
				String::NewFromUtf8(isolate, this->data), 
				Integer::New(isolate, this->callbackId),
			};

			Persistent<Function, CopyablePersistentTraits<Function> > *cb =
				(Persistent<Function, CopyablePersistentTraits<Function> > *)this->node_callback;
			Local<Function> callback = Local<Function>::New(isolate, *cb);
			callback->Call(isolate->GetCurrentContext()->Global(), argc, argv);

			this->free_device_data();
			uv_mutex_unlock(&async_lock);
		}

		void triggerDeviceChangeEvent(void)
		{
			Isolate* isolate = Isolate::GetCurrent();
			HandleScope scope(isolate);

			const unsigned argc = 4;
			Local<Value> argv[argc] = {
				Integer::New(isolate, this->deviceId),
				Integer::New(isolate, this->changeEvent),
				Integer::New(isolate, this->changeType),
				Integer::New(isolate, this->callbackId),
			};

			Persistent<Function, CopyablePersistentTraits<Function> > *cb =
				(Persistent<Function, CopyablePersistentTraits<Function> > *)this->node_callback;
			Local<Function> callback = Local<Function>::New(isolate, *cb);
			callback->Call(isolate->GetCurrentContext()->Global(), argc, argv);

			uv_mutex_unlock(&async_lock);
		}

		void triggerControllerEvent(void)
		{
			Isolate* isolate = Isolate::GetCurrent();
			HandleScope scope(isolate);

			const unsigned argc = 5;
			Local<Value> argv[argc] = {
				Integer::New(isolate, this->controllerId),
				Integer::New(isolate, this->changeEvent),
				Integer::New(isolate, this->changeType),
				String::NewFromUtf8(isolate, this->newValue), 
				Integer::New(isolate, this->callbackId),
			};

			Persistent<Function, CopyablePersistentTraits<Function> > *cb =
				(Persistent<Function, CopyablePersistentTraits<Function> > *)this->node_callback;
			Local<Function> callback = Local<Function>::New(isolate, *cb);
			callback->Call(isolate->GetCurrentContext()->Global(), argc, argv);

			this->free_controller_data();
			uv_mutex_unlock(&async_lock);
		}
};

void _tdUnregisterCallback(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	int id, ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	ret = tdUnregisterCallback(id);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}
}

void _tdGetNumberOfDevices(const FunctionCallbackInfo<Value>& args)
{
	Isolate *isolate = Isolate::GetCurrent();
	int intNumberOfDevices;

	if (args.Length() != 0) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	intNumberOfDevices = tdGetNumberOfDevices();
	args.GetReturnValue().Set(intNumberOfDevices);
}

void _tdClose(const FunctionCallbackInfo<Value>& args)
{
	Isolate *isolate = Isolate::GetCurrent();
	if (args.Length() != 0) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	tdClose();
}

void _tdTurnOn(const FunctionCallbackInfo<Value>& args)
{
	Isolate *isolate = Isolate::GetCurrent();
	int id, ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->Int32Value();
	ret = tdTurnOn(id); 

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}
}

void _tdTurnOff(const FunctionCallbackInfo<Value>& args)
{
	Isolate *isolate = Isolate::GetCurrent();
	int id, ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->Int32Value();
	ret = tdTurnOff(id); 

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}
}

void sensor_event_callback(const char *protocol, const char *model, int id, int dataType,
		const char *value, int timestamp,
		int callbackId, void  *context)
{

	uv_mutex_lock(&async_lock);

	Sensor_context *ctx = (Sensor_context *)async.data;
	ctx->node_callback = context;
	ctx->set_triggerSensorEvent();
	ctx->set_sensor_data(protocol, model, id, dataType, value, timestamp,
			callbackId, context);

	uv_async_send(&async);
}

void raw_device_event_callback(const char *data, int controllerId,
		int callbackId, void *context)
{
	uv_mutex_lock(&async_lock);

	Sensor_context *ctx = (Sensor_context *)async.data;
	ctx->node_callback = context;
	ctx->set_triggerRawDeviceEvent();
	ctx->set_raw_data(data, controllerId, callbackId, context);

	uv_async_send(&async);
}

void device_event_callback(int deviceId, int method, const char *data,
		int callbackId, void *context)
{
	uv_mutex_lock(&async_lock);

	Sensor_context *ctx = (Sensor_context *)async.data;
	ctx->node_callback = context;
	ctx->set_triggerDeviceEvent();
	ctx->set_device_data(deviceId, method, data, callbackId, context);

	uv_async_send(&async);

}

void device_change_event_callback(int deviceId, int changeEvent,
		int changeType, int callbackId, void *context)
{
	uv_mutex_lock(&async_lock);

	Sensor_context *ctx = (Sensor_context *)async.data;
	ctx->node_callback = context;
	ctx->set_triggerDeviceChangeEvent();
	ctx->set_device_change_data(deviceId, changeEvent, changeType,
			callbackId, context);

	uv_async_send(&async);
}

void controller_event(int controllerId, int changeEvent,
		int changeType, const char *newValue,
		int callbackId, void *context)
{
	uv_mutex_lock(&async_lock);

	Sensor_context *ctx = (Sensor_context *)async.data;
	ctx->node_callback = context;
	ctx->set_triggerControllerEvent();
	ctx->set_controller_data(controllerId, changeEvent, changeEvent,
			newValue, callbackId, context);

	uv_async_send(&async);
}

void trigger_node_event(uv_async_t *handle)
{
	Sensor_context *ctx = (Sensor_context *)handle->data;
	ctx->trigger_event();
}

void _tdRegisterSensorEvent(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	v8::Local<v8::Function> cb;
	int ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsFunction()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	cb = v8::Local<v8::Function>::Cast(args[0]);

	void *test = new Persistent<Function,
	     CopyablePersistentTraits<Function> >(isolate, cb);

	ret = tdRegisterSensorEvent(sensor_event_callback, test);
	args.GetReturnValue().Set(ret);
}

void _tdRegisterRawDeviceEvent(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	v8::Local<v8::Function> cb;
	
	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsFunction()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	cb = v8::Local<v8::Function>::Cast(args[0]);

	void *test = new Persistent<Function,
	     CopyablePersistentTraits<Function> >(isolate, cb);

	tdRegisterRawDeviceEvent(raw_device_event_callback, test);
}

void _tdBell(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, ret;
	
	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	ret = tdBell(id);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}
}

void _tdDim(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	unsigned char level;
	int id, ret;
	
	if (args.Length() != 2) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	level = args[1]->NumberValue();

	ret = tdDim(id, level);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}
}

void _tdExecute(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, ret;
	
	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();

	ret = tdExecute(id);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}
}

void _tdUp(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, ret;
	
	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}
	id = args[0]->NumberValue();

	ret = tdUp(id);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}
}

void _tdDown(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, ret;
	
	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}
	id = args[0]->NumberValue();

	ret = tdDown(id);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}
}

void _tdStop(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, ret;
	
	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}
	id = args[0]->NumberValue();

	ret = tdStop(id);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}
}

void _tdLearn(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, ret;
	
	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}
	id = args[0]->NumberValue();

	ret = tdLearn(id);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}
}

void _tdLastSentCommand(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, methodsSupported, ret;
	
	if (args.Length() != 2) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}
	id = args[0]->NumberValue();
	methodsSupported = args[0]->NumberValue();

	ret = tdLastSentCommand(id, methodsSupported);

	args.GetReturnValue().Set(ret);
}

void _tdLastSentValue(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id;
	char *ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	ret = tdLastSentValue(id);

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));

	tdReleaseString(ret);
}

void _tdGetDeviceId(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int intDeviceIndex, ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	intDeviceIndex = args[0]->NumberValue();
	ret = tdGetDeviceId(intDeviceIndex);

	if (ret == -1) {
		isolate->ThrowException(Exception::Error(
					String::NewFromUtf8(isolate, "Can't find device")));
		return;
	}

	args.GetReturnValue().Set(ret);
}

void _tdGetDeviceType(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}
	id = args[0]->NumberValue();

	ret = tdGetDeviceType(id);
	args.GetReturnValue().Set(ret);
}

void _tdGetName(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id;
	char *ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	ret = tdGetName(id);

	if (ret[0] != 0)
		args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));
	else
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Can't find device")));

	tdReleaseString(ret);
}

void _tdSetName(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id;
	bool ret;

	if (args.Length() != 2) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	String::Utf8Value name(args[1]->ToString());

	ret = tdSetName(id, *name);

	args.GetReturnValue().Set(ret);
}

void _tdGetProtocol(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id;
	char *ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	ret = tdGetProtocol(id);

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));

	tdReleaseString(ret);
}

void _tdSetProtocol(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id;
	bool ret;

	if (args.Length() != 2) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	String::Utf8Value strProtocol(args[1]->ToString());
	ret = tdSetProtocol(id, *strProtocol);

	args.GetReturnValue().Set(ret);
}

void _tdGetModel(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id = args[0]->NumberValue();
	char *ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	ret = tdGetModel(id);

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));

	tdReleaseString(ret);
}

void _tdSetModel(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id;
	bool ret;

	if (args.Length() != 2) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	String::Utf8Value strModel(args[1]->ToString());
	ret = tdSetModel(id, *strModel);

	args.GetReturnValue().Set(ret);
}

void _tdSetDeviceParameter(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id;
	bool ret;

	if (args.Length() != 3) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsString() || !args[2]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	String::Utf8Value strName(args[1]->ToString());
	String::Utf8Value strValue(args[2]->ToString());
	ret = tdSetDeviceParameter(id, *strName, *strValue);

	args.GetReturnValue().Set(ret);
}

void _tdGetDeviceParameter(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id;
	char *ret;

	if (args.Length() != 3) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsString() || !args[2]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	String::Utf8Value strName(args[1]->ToString());
	String::Utf8Value strValue(args[2]->ToString());
	ret = tdGetDeviceParameter(id, *strName, *strValue);

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));

	tdReleaseString(ret);
}

void _tdAddDevice(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	int ret;

	if (args.Length() != 0) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	ret = tdAddDevice();

	if (ret < 0) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}

	args.GetReturnValue().Set(ret);
}

void _tdRemoveDevice(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id;
	bool ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	ret = tdRemoveDevice(id);
	args.GetReturnValue().Set(ret);
}

void _tdMethods(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, methodsSupported, ret;

	if (args.Length() != 2) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	methodsSupported = args[1]->NumberValue();
	ret = tdMethods(id, methodsSupported);

	args.GetReturnValue().Set(ret);
}

void _tdGetErrorString(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int intErrNo;
	char *ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	intErrNo = args[0]->NumberValue();
	ret = tdGetErrorString(intErrNo);

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));

	tdReleaseString(ret);
}

void _tdSendRawCommand(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	String::Utf8Value command(args[0]->ToString());
	ret = tdSendRawCommand(*command, 0 /* reserved for future use in the API */);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}

	args.GetReturnValue().Set(ret);
}

void _tdConnectTellStickController(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int vid, pid;

	if (args.Length() != 3) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	vid = args[0]->NumberValue();
	pid = args[1]->NumberValue();
	String::Utf8Value serial(args[2]->ToString());

	tdConnectTellStickController(vid, pid, *serial);
}

void _tdDisconnectTellStickController(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int vid, pid;

	if (args.Length() != 3) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	vid = args[0]->NumberValue();
	pid = args[1]->NumberValue();
	String::Utf8Value serial(args[2]->ToString());

	tdDisconnectTellStickController(vid, pid, *serial);
}

void _tdSensor(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	char protocol[1024], model[1024];
	int id, dataTypes, ret;
	Local<Object> obj = Object::New(isolate);

	if (args.Length() != 0) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	ret = tdSensor(protocol, sizeof(protocol), model, sizeof(model),
			&id, &dataTypes);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}

	obj->Set(String::NewFromUtf8(isolate, "protocol"),
			String::NewFromUtf8(isolate, protocol));
	obj->Set(String::NewFromUtf8(isolate, "model"),
			String::NewFromUtf8(isolate, model));
	obj->Set(String::NewFromUtf8(isolate, "id"),
			Integer::New(isolate, id));
	obj->Set(String::NewFromUtf8(isolate, "dataTypes"),
			Integer::New(isolate, dataTypes));

	args.GetReturnValue().Set(obj);
}

void _tdSensorValue(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Local<Object> obj = Object::New(isolate);
	int id, dataType, ret, timestamp;
	char value[1024];

	if (args.Length() != 3) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsString() || !args[1]->IsString() ||
			!args[2]->IsNumber() || !args[3]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	String::Utf8Value protocol(args[0]->ToString());
	String::Utf8Value model(args[1]->ToString());
	id = args[2]->NumberValue();
	dataType = args[3]->NumberValue();
	ret = tdSensorValue(*protocol, *model, id, dataType,
			value, sizeof(value), &timestamp);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}

	obj->Set(String::NewFromUtf8(isolate, "value"),
			String::NewFromUtf8(isolate, value));
	obj->Set(String::NewFromUtf8(isolate, "timestamp"),
			Integer::New(isolate, timestamp));

	args.GetReturnValue().Set(obj);
}

void _tdSetControllerValue(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, ret;

	if (args.Length() != 3) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsString() || !args[2]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	String::Utf8Value name(args[1]->ToString());
	String::Utf8Value value(args[2]->ToString());

	ret = tdSetControllerValue(id, *name, *value) == TELLSTICK_SUCCESS;

	args.GetReturnValue().Set(ret);
}

void _tdRemoveController(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, ret;

	if (args.Length() != 1) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();

	ret = tdRemoveController(id);

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}
}

void _tdController(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	char name[1024];
	int controllerId, controllerType, available;
	int ret;
	Local<Object> obj = Object::New(isolate);

	if (args.Length() != 0) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	ret = tdController(&controllerId, &controllerType, name, sizeof(name),
			&available);

	if (ret != TELLSTICK_SUCCESS) {
		args.GetReturnValue().SetUndefined();
		return;
	}

	obj->Set(String::NewFromUtf8(isolate, "controllerId"),
			Integer::New(isolate, controllerId));
	obj->Set(String::NewFromUtf8(isolate, "controllerType"),
			Integer::New(isolate, controllerType));
	obj->Set(String::NewFromUtf8(isolate, "name"),
			String::NewFromUtf8(isolate, name));
	obj->Set(String::NewFromUtf8(isolate, "available"),
			Integer::New(isolate, available));

	args.GetReturnValue().Set(obj);
}

void _tdControllerValue(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id, ret;
	char value[1024];

	if (args.Length() != 2) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong number of arguments")));
		return;
	}

	if (!args[0]->IsNumber() || !args[1]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
					String::NewFromUtf8(isolate, "Wrong arguments")));
		return;
	}

	id = args[0]->NumberValue();
	String::Utf8Value name(args[1]->ToString());
	ret = tdControllerValue(id, *name, value, sizeof(value));

	if (ret != TELLSTICK_SUCCESS) {
		THROW_TELLSTICK_EXCEPTION(isolate, ret);
		return;
	}

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, value));
}

void export_defines(Handle<Object> exports)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope handle_scope(isolate);

#define OBJ_SET(obj, name, val) (obj)->Set(String::NewFromUtf8(isolate, (name)), \
		Integer::New(isolate, (val)))
	Local<Object> method_flags = Object::New(isolate);
	OBJ_SET(method_flags, "turnon", TELLSTICK_TURNON);
	OBJ_SET(method_flags, "turnoff", TELLSTICK_TURNOFF);
	OBJ_SET(method_flags, "bell", TELLSTICK_BELL);
	OBJ_SET(method_flags, "toggle", TELLSTICK_TOGGLE);
	OBJ_SET(method_flags, "dim", TELLSTICK_DIM);
	OBJ_SET(method_flags, "learn", TELLSTICK_LEARN);
	OBJ_SET(method_flags, "execute", TELLSTICK_EXECUTE);
	OBJ_SET(method_flags, "up", TELLSTICK_UP);
	OBJ_SET(method_flags, "down", TELLSTICK_DOWN);
	OBJ_SET(method_flags, "stop", TELLSTICK_STOP);
	exports->Set(String::NewFromUtf8(isolate, "method_flags"), method_flags);

	Local<Object> device_types = Object::New(isolate);
	OBJ_SET(device_types, "device", TELLSTICK_TYPE_DEVICE);
	OBJ_SET(device_types, "group", TELLSTICK_TYPE_GROUP);
	OBJ_SET(device_types, "scene", TELLSTICK_TYPE_SCENE);
	exports->Set(String::NewFromUtf8(isolate, "device_types"), device_types);

	Local<Object> sensor_value_types = Object::New(isolate);
	OBJ_SET(sensor_value_types, "temperature", TELLSTICK_TEMPERATURE);
	OBJ_SET(sensor_value_types, "humidity", TELLSTICK_HUMIDITY);
	OBJ_SET(sensor_value_types, "rainrate", TELLSTICK_RAINRATE);
	OBJ_SET(sensor_value_types, "raintotal", TELLSTICK_RAINTOTAL);
	OBJ_SET(sensor_value_types, "winddirection", TELLSTICK_WINDDIRECTION);
	OBJ_SET(sensor_value_types, "windaverage", TELLSTICK_WINDAVERAGE);
	OBJ_SET(sensor_value_types, "windgust", TELLSTICK_WINDGUST);
	exports->Set(String::NewFromUtf8(isolate, "sensor_value_types"), sensor_value_types);

	Local<Object> controller_type = Object::New(isolate);
	OBJ_SET(controller_type, "tellstick", TELLSTICK_CONTROLLER_TELLSTICK);
	OBJ_SET(controller_type, "tellstick_duo", TELLSTICK_CONTROLLER_TELLSTICK_DUO);
	OBJ_SET(controller_type, "tellstick_net", TELLSTICK_CONTROLLER_TELLSTICK_NET);
	exports->Set(String::NewFromUtf8(isolate, "controller_type"), controller_type);

	Local<Object> device_changes = Object::New(isolate);
	OBJ_SET(device_changes, "added", TELLSTICK_DEVICE_ADDED);
	OBJ_SET(device_changes, "changed", TELLSTICK_DEVICE_CHANGED);
	OBJ_SET(device_changes, "removed", TELLSTICK_DEVICE_REMOVED);
	OBJ_SET(device_changes, "state_changed", TELLSTICK_DEVICE_STATE_CHANGED);
	exports->Set(String::NewFromUtf8(isolate, "device_changes"), device_changes);

	Local<Object> change_types = Object::New(isolate);
	OBJ_SET(change_types, "name", TELLSTICK_CHANGE_NAME);
	OBJ_SET(change_types, "protocol", TELLSTICK_CHANGE_PROTOCOL);
	OBJ_SET(change_types, "model", TELLSTICK_CHANGE_MODEL);
	OBJ_SET(change_types, "method", TELLSTICK_CHANGE_METHOD);
	OBJ_SET(change_types, "available", TELLSTICK_CHANGE_AVAILABLE);
	OBJ_SET(change_types, "firmware", TELLSTICK_CHANGE_FIRMWARE);
	exports->Set(String::NewFromUtf8(isolate, "change_types"), change_types);

	Local<Object> error_codes = Object::New(isolate);
	OBJ_SET(error_codes, "success", TELLSTICK_SUCCESS);
	OBJ_SET(error_codes, "not_found", TELLSTICK_ERROR_NOT_FOUND);
	OBJ_SET(error_codes, "permission_denied", TELLSTICK_ERROR_PERMISSION_DENIED);
	OBJ_SET(error_codes, "device_not_found", TELLSTICK_ERROR_DEVICE_NOT_FOUND);
	OBJ_SET(error_codes, "method_not_supported", TELLSTICK_ERROR_METHOD_NOT_SUPPORTED);
	OBJ_SET(error_codes, "communication", TELLSTICK_ERROR_COMMUNICATION);
	OBJ_SET(error_codes, "connecting_service", TELLSTICK_ERROR_CONNECTING_SERVICE);
	OBJ_SET(error_codes, "unknown_response", TELLSTICK_ERROR_UNKNOWN_RESPONSE);
	OBJ_SET(error_codes, "syntax", TELLSTICK_ERROR_SYNTAX);
	OBJ_SET(error_codes, "broken_pipe", TELLSTICK_ERROR_BROKEN_PIPE);
	OBJ_SET(error_codes, "communicating_service", TELLSTICK_ERROR_COMMUNICATING_SERVICE);
	OBJ_SET(error_codes, "unknown", TELLSTICK_ERROR_UNKNOWN);
	exports->Set(String::NewFromUtf8(isolate, "error_codes"), error_codes);
#undef OBJ_SET
}

void init(Handle<Object> exports)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope handle_scope(isolate);

	NODE_SET_METHOD(exports, "tdClose", _tdClose);
	NODE_SET_METHOD(exports, "tdTurnOff", _tdTurnOff);
	NODE_SET_METHOD(exports, "tdTurnOn", _tdTurnOn);
	NODE_SET_METHOD(exports, "tdGetNumberOfDevices", _tdGetNumberOfDevices);
	NODE_SET_METHOD(exports, "tdRegisterSensorEvent", _tdRegisterSensorEvent);
	NODE_SET_METHOD(exports, "tdRegisterRawDeviceEvent", _tdRegisterRawDeviceEvent);
	NODE_SET_METHOD(exports, "tdUnregisterCallback", _tdUnregisterCallback);
	NODE_SET_METHOD(exports, "tdBell", _tdBell);
	NODE_SET_METHOD(exports, "tdDim", _tdDim);
	NODE_SET_METHOD(exports, "tdExecute", _tdExecute);
	NODE_SET_METHOD(exports, "tdUp", _tdUp);
	NODE_SET_METHOD(exports, "tdDown", _tdDown);
	NODE_SET_METHOD(exports, "tdStop", _tdStop);
	NODE_SET_METHOD(exports, "tdLastSentCommand", _tdLastSentCommand);
	NODE_SET_METHOD(exports, "tdLastSentValue", _tdLastSentValue);
	NODE_SET_METHOD(exports, "tdGetDeviceId", _tdGetDeviceId);
	NODE_SET_METHOD(exports, "tdGetDeviceType", _tdGetDeviceType);
	NODE_SET_METHOD(exports, "tdGetName", _tdGetName);
	NODE_SET_METHOD(exports, "tdSetName", _tdSetName);
	NODE_SET_METHOD(exports, "tdGetProtocol", _tdGetProtocol);
	NODE_SET_METHOD(exports, "tdSetProtocol", _tdSetProtocol);
	NODE_SET_METHOD(exports, "tdGetModel", _tdGetModel);
	NODE_SET_METHOD(exports, "tdSetModel", _tdSetModel);
	NODE_SET_METHOD(exports, "tdSetDeviceParameter", _tdSetDeviceParameter);
	NODE_SET_METHOD(exports, "tdGetDeviceParameter", _tdGetDeviceParameter);
	NODE_SET_METHOD(exports, "tdAddDevice", _tdAddDevice);
	NODE_SET_METHOD(exports, "tdRemoveDevice", _tdRemoveDevice);
	NODE_SET_METHOD(exports, "tdMethods", _tdMethods);
	NODE_SET_METHOD(exports, "tdGetErrorString", _tdGetErrorString);
	NODE_SET_METHOD(exports, "tdSendRawCommand", _tdSendRawCommand);
	NODE_SET_METHOD(exports, "tdConnectTellStickController", _tdConnectTellStickController);
	NODE_SET_METHOD(exports, "tdDisconnectTellStickController", _tdDisconnectTellStickController);
	NODE_SET_METHOD(exports, "tdSensor", _tdSensor);
	NODE_SET_METHOD(exports, "tdSensorValue", _tdSensorValue);
	NODE_SET_METHOD(exports, "tdController", _tdController);
	NODE_SET_METHOD(exports, "tdControllerValue", _tdControllerValue);
	NODE_SET_METHOD(exports, "tdSetControllerValue", _tdSetControllerValue);
	NODE_SET_METHOD(exports, "tdRemoveController", _tdRemoveController);

	export_defines(exports);

	tdInit();

	loop = uv_default_loop();	
	uv_mutex_init(&async_lock);

	Sensor_context *ctx = new Sensor_context();
	uv_async_init(loop, &async, trigger_node_event);	

	async.data = ctx;
}

NODE_MODULE(addon, init)

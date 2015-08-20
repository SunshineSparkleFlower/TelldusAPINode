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
	int id = args[0]->NumberValue();
	tdUnregisterCallback(id);
}

void _tdGetNumberOfDevices(const FunctionCallbackInfo<Value>& args)
{
	int intNumberOfDevices = tdGetNumberOfDevices();
	args.GetReturnValue().Set(intNumberOfDevices);
}

void _tdClose(const FunctionCallbackInfo<Value>& args)
{
	tdClose();
}

void _tdTurnOn(const FunctionCallbackInfo<Value>& args)
{
	printf("id: %d\n", args[0]->Int32Value()); 	
	printf("turnOn: %d\n", tdTurnOn(args[0]->Int32Value())); 
}

void _tdTurnOff(const FunctionCallbackInfo<Value>& args)
{
	tdTurnOff(args[0]->Int32Value()); 
}

void sensor_event_callback(const char *protocol, const char *model, int id, int dataType,
		const char *value, int timestamp,
		int callbackId, void  *context){

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

	v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(args[0]);

	void *test = new Persistent<Function,
	     CopyablePersistentTraits<Function> >(isolate, cb);

	tdRegisterSensorEvent(sensor_event_callback, test);
}

void _tdRegisterRawDeviceEvent(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(args[0]);

	void *test = new Persistent<Function,
	     CopyablePersistentTraits<Function> >(isolate, cb);

	tdRegisterRawDeviceEvent(raw_device_event_callback, test);
}

void _tdBell(const FunctionCallbackInfo<Value> &args)
{
	int id = args[0]->NumberValue();
	tdBell(id);
}

void _tdDim(const FunctionCallbackInfo<Value> &args)
{
	int id = args[0]->NumberValue();
	unsigned char level = args[1]->NumberValue();

	tdDim(id, level);
}

void _tdExecute(const FunctionCallbackInfo<Value> &args)
{
	int id = args[0]->NumberValue();

	tdExecute(id);
}

void _tdUp(const FunctionCallbackInfo<Value> &args)
{
	int id = args[0]->NumberValue();

	tdUp(id);
}

void _tdDown(const FunctionCallbackInfo<Value> &args)
{
	int id = args[0]->NumberValue();

	tdDown(id);
}

void _tdStop(const FunctionCallbackInfo<Value> &args)
{
	int id = args[0]->NumberValue();

	tdStop(id);
}

void _tdLearn(const FunctionCallbackInfo<Value> &args)
{
	int id = args[0]->NumberValue();

	tdLearn(id);
}

void _tdLastSentCommand(const FunctionCallbackInfo<Value> &args)
{
	int id = args[0]->NumberValue(), ret;
	int methodsSupported = args[0]->NumberValue();

	ret = tdLastSentCommand(id, methodsSupported);

	args.GetReturnValue().Set(ret);
}

void _tdLastSentValue(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id = args[0]->NumberValue();
	char *ret;

	ret = tdLastSentValue(id);

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));

	tdReleaseString(ret);
}

void _tdGetDeviceId(const FunctionCallbackInfo<Value> &args)
{
	int intDeviceIndex = args[0]->NumberValue(), ret;

	ret = tdGetDeviceId(intDeviceIndex);
	args.GetReturnValue().Set(ret);
}

void _tdGetDeviceType(const FunctionCallbackInfo<Value> &args)
{
	int id = args[0]->NumberValue(), ret;

	ret = tdGetDeviceType(id);
	args.GetReturnValue().Set(ret);
}

void _tdGetName(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id = args[0]->NumberValue();
	char *ret;

	ret = tdGetName(id);

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));

	tdReleaseString(ret);
}

void _tdSetName(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id = args[0]->NumberValue();
	String::Utf8Value name(args[1]->ToString());
	bool ret;

	ret = tdSetName(id, *name);

	args.GetReturnValue().Set(ret);
}

void _tdGetProtocol(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id = args[0]->NumberValue();
	char *ret;

	ret = tdGetProtocol(id);

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));

	tdReleaseString(ret);
}

void _tdSetProtocol(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id = args[0]->NumberValue();
	String::Utf8Value strProtocol(args[1]->ToString());
	bool ret;

	ret = tdSetProtocol(id, *strProtocol);

	args.GetReturnValue().Set(ret);
}

void _tdGetModel(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id = args[0]->NumberValue();
	char *ret;

	ret = tdGetModel(id);

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));

	tdReleaseString(ret);
}

void _tdSetModel(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id = args[0]->NumberValue();
	String::Utf8Value strModel(args[1]->ToString());
	bool ret;

	ret = tdSetModel(id, *strModel);

	args.GetReturnValue().Set(ret);
}

void _tdSetDeviceParameter(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id = args[0]->NumberValue();
	String::Utf8Value strName(args[1]->ToString());
	String::Utf8Value strValue(args[2]->ToString());
	bool ret;

	ret = tdSetDeviceParameter(id, *strName, *strValue);

	args.GetReturnValue().Set(ret);
}

void _tdGetDeviceParameter(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id = args[0]->NumberValue();
	String::Utf8Value strName(args[1]->ToString());
	String::Utf8Value strValue(args[2]->ToString());
	char *ret;

	ret = tdGetDeviceParameter(id, *strName, *strValue);

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));

	tdReleaseString(ret);
}

void _tdAddDevice(const FunctionCallbackInfo<Value> &args)
{
	int ret = tdAddDevice();
	args.GetReturnValue().Set(ret);
}

void _tdRemoveDevice(const FunctionCallbackInfo<Value> &args)
{
	int id = args[0]->NumberValue();
	bool ret = tdRemoveDevice(id);
	args.GetReturnValue().Set(ret);
}

void _tdMethods(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int id = args[0]->NumberValue();
	int methodsSupported = args[1]->NumberValue();
	int ret;

	ret = tdMethods(id, methodsSupported);

	args.GetReturnValue().Set(ret);
}

void _tdGetErrorString(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	int intErrNo = args[0]->NumberValue();
	char *ret;

	ret = tdGetErrorString(intErrNo);

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));

	tdReleaseString(ret);
}

void _tdSendRawCommand(const FunctionCallbackInfo<Value> &args)
{
	String::Utf8Value command(args[0]->ToString());
	int reserved = args[1]->NumberValue(), ret;

	ret = tdSendRawCommand(*command, reserved);

	args.GetReturnValue().Set(ret);
}

void _tdConnectTellStickController(const FunctionCallbackInfo<Value> &args)
{
	int vid = args[0]->NumberValue();
	int pid = args[1]->NumberValue();
	String::Utf8Value serial(args[2]->ToString());

	tdConnectTellStickController(vid, pid, *serial);
}

void _tdDisconnectTellStickController(const FunctionCallbackInfo<Value> &args)
{
	int vid = args[0]->NumberValue();
	int pid = args[1]->NumberValue();
	String::Utf8Value serial(args[2]->ToString());

	tdDisconnectTellStickController(vid, pid, *serial);
}

void _tdSensor(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	char protocol[1024], model[1024];
	int id, dataTypes;
	int ret;
	Local<Object> obj = Object::New(isolate);

	ret = tdSensor(protocol, sizeof(protocol), model, sizeof(model),
			&id, &dataTypes);

	if (ret != TELLSTICK_SUCCESS) {
		args.GetReturnValue().SetUndefined();
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
	String::Utf8Value protocol(args[0]->ToString());
	String::Utf8Value model(args[1]->ToString());
	int id = args[2]->NumberValue();
	int dataType = args[3]->NumberValue();
	char value[1024];
	int ret, timestamp;

	ret = tdSensorValue(*protocol, *model, id, dataType,
			value, sizeof(value), &timestamp);

	if (ret != TELLSTICK_SUCCESS) {
		args.GetReturnValue().SetUndefined();
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

	int id = args[0]->NumberValue();
	String::Utf8Value name(args[1]->ToString());
	String::Utf8Value value(args[2]->ToString());

	int ret;

	ret = tdSetControllerValue(id, *name, *value);

	args.GetReturnValue().Set(ret);
}

void _tdRemoveController(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	int id = args[0]->NumberValue(), ret;

	ret = tdRemoveController(id);

	args.GetReturnValue().Set(ret);
}

void _tdController(const FunctionCallbackInfo<Value> &args)
{
	Isolate *isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	char name[1024];
	int controllerId, controllerType, available;
	int ret;
	Local<Object> obj = Object::New(isolate);

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
	Local<Object> obj = Object::New(isolate);
	String::Utf8Value protocol(args[0]->ToString());
	String::Utf8Value model(args[1]->ToString());
	int id = args[2]->NumberValue();
	int dataType = args[3]->NumberValue();
	char value[1024];
	int ret, timestamp;

	ret = tdSensorValue(*protocol, *model, id, dataType,
			value, sizeof(value), &timestamp);

	if (ret != TELLSTICK_SUCCESS) {
		args.GetReturnValue().SetUndefined();
		return;
	}

	obj->Set(String::NewFromUtf8(isolate, "value"),
			String::NewFromUtf8(isolate, value));
	obj->Set(String::NewFromUtf8(isolate, "timestamp"),
			Integer::New(isolate, timestamp));

	args.GetReturnValue().Set(obj);
}

void init(Handle<Object> exports)
{
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

	tdInit();

	loop = uv_default_loop();	
	uv_mutex_init(&async_lock);

	Sensor_context *ctx = new Sensor_context();
	uv_async_init(loop, &async, trigger_node_event);	

	async.data = ctx;
}

NODE_MODULE(addon, init)

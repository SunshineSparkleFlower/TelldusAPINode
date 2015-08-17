// hello.js
var addon = require('./build/Release/addon');


function sensorListen(protocol, model, id, dataType,  value, timestamp, callbackId){
	console.log('sensorevent');
	console.log(id);

}
function modelListen(protocol, model, id, datatype, value, timestamp, callbackid){
	console.log("protocol: " + protocol);
	console.log("value   : " + value);
}

function rawListen(data, controllerId, callbackId) {
	console.log(data)
	//console.log("controllerId: " + controllerId)
	//console.log("callbackId: " + callbackId)
}

/*
console.log(addon.hello()); // 'world'
console.log(addon.tdGetNumberOfDevices());

for(var i = 0; i < 1; i++){
	var sleep = require('sleep');
		//sleep.sleep(2);
	addon.tdTurnOff(2);

	//sleep.sleep(2);
	addon.tdTurnOn(2);
}
*/
//addon.tdRegisterSensorEvent(modelListen);
addon.tdRegisterRawDeviceEvent(rawListen);

/*
setTimeout(function () {
	console.log('bla');
}, 30000);

addon.tdTurnOff(2);

addon.tdclose();

*/

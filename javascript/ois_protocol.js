
function MakeFourCC( cmd )
{
	return cmd.charCodeAt(0) | (cmd.charCodeAt(1)<<8) | (cmd.charCodeAt(2)<<16) | (cmd.charCodeAt(3)<<24);
}

var OisLog = Object.freeze({
	"Out":1,
	"In":2,
	"Error":3
});
var OisType = Object.freeze({
	"Boolean":1,
	"Number":2,
	"Fraction":3
});
var OisDeviceState = Object.freeze({
	"Handshaking":1,
	"Synchronisation":2,
	"Active":3
});
var OisCommands = Object.freeze({
	"DENn":MakeFourCC("DEN\n"),
	"ACKn":MakeFourCC("ACK\n"),
	"ACK2":MakeFourCC("ACK="),
	"ENDn":MakeFourCC("END\n")
});
	
function OisState(url, deviceName, pid, vid, commands, inputs, outputs, version, logFunction)
{
	if (!(this instanceof OisState)) 
		return new OisState(url, deviceName, pid, vid, commands, inputs, outputs, version);
	
	var log = logFunction ? logFunction : function() {};
		
	this.wsOpen = false;
	this.maxVersion = version > 0 ? version : 2;
	this.version = 0;
	this.lastHandshakeTime = new Date();
	this.deviceName = deviceName;
	this.pid = pid;
	this.vid = vid;
	this.commands = commands;
	this.inputs = inputs;
	this.outputs = outputs;
	this.inputIndices = Object.keys(inputs);
	this.outputIndices = Object.keys(outputs);
	this.inputValues = {};
	this.inputCallbacks = {};
	for(var key in inputs) 
		this.inputValues[key] = 0;
	
	var self = this;
	var Reset;
	
	function CommandChannelFromIndex(idx)
	{
		return idx;
	};
	function CommandChannelFromName(name)
	{
		return self.commands.indexOf(name);
	};
	function InputChannelFromIndex(idx)
	{
		return idx >= 0 ? idx + self.commands.length : idx;
	};
	function InputChannelFromName(name)
	{
		return InputChannelFromIndex( self.inputIndices.indexOf(name) );
	};
	function InputIndexFromChannel(channel)
	{
		var idx = channel - self.commands.length;
		return idx >= 0 && idx < self.inputIndices.length ? idx : -1;
	};
	function OutputChannelFromIndex(idx)
	{
		return idx >= 0 ? idx + self.commands.length + self.inputIndices.length : idx;
	}
	function OutputChannelFromName(name)
	{
		return OutputChannelFromIndex( self.outputIndices.indexOf(name) );
	};
	
	this.Disconnect = function()
	{
		if( self.ws )
		{
			self.ws.close();
			clearInterval(this.intervalId);
		}
	}
	this.SetOutput = function(name, value)
	{
		var index = OutputChannelFromName(name);
		if( index < 0 )
		{
			log(OisLog.Error, name);
			return false;
		}
		switch( self.outputs[name] )
		{
		default:
			return false;
		case OisType.Boolean:
			value = value!=0 ? 1 : 0;
			break;
		case OisType.Number:
			value = Math.min(32767, Math.max(-32768, Math.round(value)));
			break;
		case OisType.Fraction:
			value = Math.min(32767, Math.max(-32768, Math.round(value*100)));
			break;
		}
		self.writeBuffer += index+"="+value+"\n";
		return true;
	};
	this.GetInput = function(name)
	{
		return self.inputValues[name];
	};
	this.OnChanged = function(inputName, callback)
	{
		if( !(inputName in self.inputCallbacks) ) 
			self.inputCallbacks[inputName] = [];
		self.inputCallbacks[inputName].push(callback);
	};
	this.ExecuteCommand = function(name)
	{
		var index = CommandChannelFromName(name);
		if( index < 0 )
			return false;
		self.writeBuffer += "EXC="+index+"\n";
		return true;
	};
	
	function OnPoll()
	{
		if( self.ws.readyState != 1 )
		{
			if( self.ws.readyState == 3 )
				Reset();
			return;
		}
		
		if( self.deviceState == OisDeviceState.Handshaking )
		{
			var now = new Date();
			var timeDiff = now - self.lastHandshakeTime;
			if( timeDiff > 1000 )
			{
				self.lastHandshakeTime = now;
				var msg = "SYN="+self.version+"\n";
				self.ws.send(msg);
				log(OisLog.Out, msg);
			}
		}
		else if( self.deviceState == OisDeviceState.Synchronisation )
		{
			var buffer = "PID="+self.pid+","+self.vid+","+self.deviceName+"\n";
			for( var i=0; i!=self.commands.length; ++i )
			{
				buffer += "CMD="+self.commands[i]+","+CommandChannelFromIndex(i)+"\n";
			}
			for( var i=0; i!=self.inputIndices.length; ++i )
			{
				var name = self.inputIndices[i];
				switch( self.inputs[name] )
				{
				case OisType.Boolean:  buffer += "NIB="+name+","+InputChannelFromIndex(i)+"\n"; break;
				case OisType.Number:   buffer += "NIN="+name+","+InputChannelFromIndex(i)+"\n"; break;
				case OisType.Fraction: buffer += "NIF="+name+","+InputChannelFromIndex(i)+"\n"; break;
				}
			}
			if( self.version >= 2 )
			{
				for( var i=0; i!=self.outputIndices.length; ++i )
				{
					var name = self.outputIndices[i];
					switch( self.outputs[name] )
					{
					case OisType.Boolean:  buffer += "NOB="+name+","+OutputChannelFromIndex(i)+"\n"; break;
					case OisType.Number:   buffer += "NON="+name+","+OutputChannelFromIndex(i)+"\n"; break;
					case OisType.Fraction: buffer += "NOF="+name+","+OutputChannelFromIndex(i)+"\n"; break;
					}
				}
			}
			buffer += "ACT\n";
			log(OisLog.Out, buffer);
			self.ws.send(buffer);
			self.deviceState = OisDeviceState.Active;
		}
		else if( self.writeBuffer.length )
		{
			log(OisLog.Out, self.writeBuffer);
			self.ws.send(self.writeBuffer);
			self.writeBuffer = "";
		}
	};
	function ProcessCommand()
	{
		if( self.commandBuffer.length < 4 )
			return;
		var newlineIdx = self.commandBuffer.indexOf('\n');
		if( newlineIdx < 0 )
			return;
		var cmd = self.commandBuffer.substring(0, newlineIdx);
		self.commandBuffer = self.commandBuffer.substring(newlineIdx+1);
		
		log(OisLog.In, cmd);
		
		var fourcc = cmd.charCodeAt(0) | (cmd.charCodeAt(1)<<8) | (cmd.charCodeAt(2)<<16) | (cmd.charCodeAt(3)<<24);
		switch( fourcc )
		{
		case OisCommands.DENn:
		{
			if( self.version > 0 )
				self.version--;
			else
				self.version = self.maxVersion;
			break;
		}
		case OisCommands.ENDn:
		{
			Reset();
			break;
		}
		case OisCommands.ACK2:
		{
			var commaIdx = cmd.indexOf(',');
			if( commaIdx >= 0 )
			{
				self.gameTitle = cmd.substr(commaIdx+1);
				cmd = cmd.substr(0,commaIdx);
			}
			self.gameVersion = parseInt(cmd);
			self.deviceState = OisDeviceState.Synchronisation;
			break;
		}
		case OisCommands.ACKn:
		{
			self.version = 1;
			self.deviceState = OisDeviceState.Synchronisation;
			break;
		}
		default:
			var eqIdx = cmd.indexOf('=');
			if( eqIdx >= 0 )
			{
				var value   = parseInt(cmd.substr(eqIdx+1));
				var channel = parseInt(cmd.substr(0,eqIdx));
				var idxInput = InputIndexFromChannel(channel);
				if( idxInput >= 0 )
				{
					var name = self.inputIndices[idxInput];
					switch( self.inputs[name] )
					{
					default:
						return false;
					case OisType.Boolean:
						self.inputValues[name] = value!=0 ? true : false;
						break;
					case OisType.Number:
						self.inputValues[name] = value;
						break;
					case OisType.Fraction:
						self.inputValues[name] = value / 100.0;
						break;
					}
					var inputCallbacks = self.inputCallbacks[name];
					if( inputCallbacks )
					{
						for( var i = 0; i < inputCallbacks.length; i++ )
						{
							inputCallbacks[i](self.inputValues[name], name);
						}
					}
				}
			}
		}
		
		ProcessCommand();
	}
	function OnMessage( message )
	{
		self.commandBuffer += message;
		ProcessCommand();
	};
	
	var wsReader = new FileReader();
	wsReader.onload = function(event)
	{
		OnMessage(wsReader.result);
	};
	
	function Reset()
	{
		self.lastHandshakeTime = new Date();
		self.deviceState = OisDeviceState.Handshaking;
		self.binary = false;
		self.commandBuffer = "";
		self.writeBuffer = "";
		self.gameTitle = "";
		self.gameVersion = 0;
		self.version = self.maxVersion;
		if( url != "ws:///input" )
		{
			self.ws = new WebSocket(url);
			self.ws.onopen = function() 
			{
				self.wsOpen = true;
			};
			self.ws.onclose = function()
			{ 
				self.wsOpen = false;
			};
			self.ws.onerror = self.ws.onclose;
			self.ws.onmessage = function(evt) 
			{ 
				wsReader.readAsText(evt.data);
			};
			log(OisLog.Out, "Connecting to "+url);
		}
	};
	Reset();
	
	this.intervalId = setInterval(function() {
		OnPoll();
	}, 100);
}

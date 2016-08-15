function terminal_manager_t(doorway_manager)
{
	if(!doorway_manager)
		return null;
	this.doorway_manager=doorway_manager;
	var _this=this;
	this.interval=setInterval(function()
	{
		_this.update();
	},200);
	this.terminals={};
	this.updates={};
}

terminal_manager_t.prototype.destroy=function()
{
	if(this.interval)
	{
		clearInterval(this.interval);
		this.interval=null;
	}
	if(this.terminals)
	{
		for(var key in this.terminals)
			this.terminals[key].destroy();
		this.terminals=null;
	}
	if(this.updates)
		this.updates=null;
	if(this.doorway_manager)
		this.doorway_manager=null;
}

terminal_manager_t.prototype.update=function()
{
	var _this=this;
	var xhr=new XMLHttpRequest();
	xhr.onreadystatechange=function()
	{
		if(xhr.readyState==4&&xhr.status==200)
		{
			try
			{
				var updates=JSON.parse(xhr.responseText);
				for(var key in updates.result)
				{
					if(!(key in _this.terminals))
					{
						_this.updates[key]=0;
						var doorway=_this.doorway_manager.add({title:key});
						var old_settings=localStorage.getItem(key);
						if(old_settings)
							doorway.load(JSON.parse(old_settings));
						_this.terminals[key]=new terminal_t(_this,doorway);
					}
					_this.updates[key]+=updates.result[key].length;
					var terminal=_this.terminals[key];
					for(var line in updates.result[key])
					{
						var current_scroll=terminal.history.scrollHeight-terminal.history.scrollTop;
						var scroll_end=(current_scroll==terminal.history.offsetHeight);
						var text=document.createTextNode(updates.result[key][line]);
						terminal.history.appendChild(text);
						var line_break=document.createElement("br");
						terminal.history.appendChild(line_break);
						if(scroll_end)
							terminal.history.scrollTop=terminal.history.scrollHeight;
					}
				}
			}
			catch(error)
			{
				console.log(error);
			}
		}
	};
	xhr.open("POST","",true);
	var data={method:"updates",params:this.updates};
	xhr.send(JSON.stringify(data));
}

terminal_manager_t.prototype.send=function(address,line)
{
	var _this=this;
	var xhr=new XMLHttpRequest();
	xhr.open("POST","",true);
	var data={method:"write",params:{address:address,line:line}};
	xhr.send(JSON.stringify(data));
}

function terminal_t(manager,doorway)
{
	if(!manager||!doorway)
		return null;
		this.manager=manager;
	this.doorway=doorway;
	var _this=this;

	this.el=document.createElement("div");
	this.doorway.win.appendChild(this.el);
	this.el.className="doorways terminal win";

	this.history=document.createElement("div");
	this.el.appendChild(this.history);
	this.history.className="doorways terminal history";

	this.input=document.createElement("input");
	this.el.appendChild(this.input);
	this.input.className="doorways terminal input";
	this.input.placeholder="Command";
	this.input.addEventListener("keyup",function(evt)
	{
		if(evt.keyCode==13)
		{
			_this.manager.send(doorway.title,this.value);
			this.value="";
		}
	});

	this.interval=setInterval(function()
	{
		localStorage.setItem(_this.doorway.title,JSON.stringify(_this.doorway.save()));
	},500);
}

terminal_t.prototype.destroy=function()
{
	if(this.doorway)
	{
		this.doorway.win.removeChild(this.el);
		this.doorway=this.el=this.input=null;
	}
	if(this.manager)
		this.manager=null;
}
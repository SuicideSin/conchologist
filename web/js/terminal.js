function terminal_manager_t(doorway_manager)
{
	if(!doorway_manager)
		return null;
	this.doorway_manager=doorway_manager;
	var _this=this;
	this.update();
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
		if(xhr.readyState==4)
		{
			if(xhr.status==200)
			{
				try
				{
					var updates=JSON.parse(xhr.responseText);
					for(var key in updates.result)
					{
						if(!(key in _this.terminals))
						{
							_this.updates[key]=0;
							var doorway=_this.doorway_manager.add
							({
								title:key,
								min_size:
								{
									w:200,
									h:200
								}
							});
							var old_settings=localStorage.getItem(key);
							if(old_settings)
								doorway.load(JSON.parse(old_settings));
							_this.terminals[key]=new terminal_t(_this,doorway);
						}
						if(updates.result[key].last_count>=_this.updates[key])
						{
							_this.updates[key]+=updates.result[key].new_lines.length;
							for(var line in updates.result[key].new_lines)
								_this.terminals[key].add_line(updates.result[key].new_lines[line]);
						}
					}
					_this.update();
				}
				catch(error)
				{
					setTimeout(function()
					{
						_this.update();
					},1000);
				}
			}
			else
			{
				setTimeout(function()
				{
					_this.update();
				},1000);
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
	this.history_lookup=[];
	this.history_ptr=-1;

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
	this.input.addEventListener("keydown",function(evt)
	{
		if(evt.keyCode==13)
		{
			_this.manager.send(doorway.title,this.value);
			this.value="";
		}
		else if(evt.keyCode==38)
		{
			evt.preventDefault();
			if(_this.history_ptr>0&&_this.history_lookup.length>0)
			{
				--_this.history_ptr;
				this.value=_this.history_lookup[_this.history_ptr];
				_this.move_cursor_end();
			}
		}
		else if(evt.keyCode==40)
		{
			evt.preventDefault();
			if(_this.history_ptr+1<=_this.history_lookup.length&&_this.history_lookup.length>0)
			{
				++_this.history_ptr;
				if(_this.history_ptr==_this.history_lookup.length)
				{
					this.value="";
				}
				else
				{
					this.value=_this.history_lookup[_this.history_ptr];
				}
				_this.move_cursor_end();
			}
		}
	});

	this.interval=setInterval(function()
	{
		localStorage.setItem(_this.doorway.title,JSON.stringify(_this.doorway.save()));
	},500);
}

terminal_t.prototype.add_line=function(line)
{
	var current_scroll=this.history.scrollHeight-this.history.scrollTop;
	var scroll_end=(current_scroll==this.history.offsetHeight);
	this.history.appendChild(document.createTextNode(line));
	this.history.appendChild(document.createElement("br"));
	if(scroll_end)
		this.history.scrollTop=this.history.scrollHeight;
	if(line.substr(0,2)=="> "&&(this.history_lookup.length==0||
		this.history_lookup[this.history_lookup.length-1]!=line.substr(2,line.length)))
		this.history_lookup.push(line.substr(2,line.length));
	this.history_ptr=this.history_lookup.length;
	if(this.history_ptr<0)
		this.history_ptr=0;
}

terminal_t.prototype.destroy=function()
{
	if(this.interval)
	{
		clearInterval(this.interval);
		this.interval=null;
	}
	if(this.doorway)
	{
		this.doorway.win.removeChild(this.el);
		this.doorway=this.el=this.input=null;
	}
	if(this.manager)
		this.manager=null;
	this.history_lookup=this.history_ptr=null;
}

terminal_t.prototype.move_cursor_end=function()
{
	var input=this.input;
	setTimeout(function()
	{
		input.selectionStart=input.selectionEnd=input.value.length;
	},0);
}
function terminal_t(doorway)
{
	if(!doorway)
		return null;
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
			_this.history.innerHTML+="> "+this.value+"<br/>";
			this.value="";
		}
	});
}

terminal_t.prototype.destroy=function()
{
	if(this.doorway)
	{
		this.doorway.win.removeChild(this.el);
		this.doorway=this.el=this.input=null;
	}
}
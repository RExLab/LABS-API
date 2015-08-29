var express = require('express');
var panel = require('./build/Release/panel.node');


var app = express();
var configured = false;

app.get('/:command', function (req, res) {

	console.log(req.params.command);


if(req.params.command == 'setup'){
	configured = panel.setup();
	res.send("1 " + configured);
	
}else if(req.params.command == 'run'){
	if (configured)
		panel.run();

    res.send("OK!");

}else if(req.params.command == 'exit'){
	if(configured){
		panel.exit('0');
		configured = false;
	}
	res.send("OK!");

}else if(req.params.command == 'update'){
	if(configured){
		console.log(panel.update(7));
	}
    res.send("OK!");

}else{
	if(configured && !isNaN(req.params.command)){
		console.log(panel.update(req.params.command));
		var data = panel.getvalues();
	    console.log(req.params.command);
		console.log(JSON.parse(data));
	    res.send(data);
	}else{
	    res.send("NOK!");

	}
}
	

});


var server = app.listen(3000, function () {

  var host = server.address().address;
  var port = server.address().port;
  
  console.log('App listening at http://%s:%s', host, port);
  //panel.setup();
  //panel.run();
  //console.log(panel.update('111'));


});


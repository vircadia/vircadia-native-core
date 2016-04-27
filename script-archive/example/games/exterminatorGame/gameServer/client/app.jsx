'use strict';

var React = require('react');
var _ = require('underscore')
var $ = require('jquery');

var UserList = React.createClass({
render: function(){
	var sortedUsers = _.sortBy(this.props.data.users, function(users){
		//Show higher scorers at top of board
		return 1 - users.score;
	});
	var users = sortedUsers.map(function(user) {

		return (
			<User username = {user.username} score = {user.score} key = {user.id}></User>
		)
	});
	return (
		<div>{users}</div>
	)
  }
});

var GameBoard = React.createClass({
   loadDataFromServer: function(data) {
		$.ajax({
			url: this.props.url,
			dataType: 'json',
			cache: false,
			success: function(data) {
				this.setState({data: data});
			}.bind(this),
			error: function(xhr, status, err) {
				console.error(this.props.url, status, err.toString());
			}.bind(this)
		});
  },
  getInitialState: function() {
  	return {data: {users: []}};
  },
  componentDidMount: function() {
  	this.loadDataFromServer();
    //set up web socket
    var path =  window.location.hostname + ":" + window.location.port;
    console.log("LOCATION ", path)
    var socketClient = new WebSocket("wss://" + path);
    var self = this;
    socketClient.onopen = function() {
        console.log("CONNECTED");
        socketClient.onmessage = function(data) {
            console.log("ON MESSAGE");
            self.setState({data: JSON.parse(data.data)});
        };
    };

  },
  render: function() {

    return (
      <div>
        <div className = "gameTitle">Kill All The Rats!</div> 
        <div className = "boardHeader">
            <div className="username">PLAYER</div>
            <div className="score" > SCORE </div>
        </div>
        <UserList data ={this.state.data}/>
      </div>
    );
  }
});

var User = React.createClass({
	render: function() {
		return (
		<div className = "entry">
			<div className="username"> {this.props.username} </div>
			<div className="score" > {this.props.score} </div>
		</div>
		);
	}
})

React.render(
  <GameBoard url = "/users" />,
  document.getElementById('app')
);
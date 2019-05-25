// TODO Will need to add WS subprotocol so we know what version is in use

/**
 * This library is used to communicate with a text game (currently only
 * MUTGOS) server via a websocket.  Clients should set the public attributes
 * to appropriate methods (as desired) before connecting.
 *
 * This library will automatically take care of reconnecting when the
 * connection is lost.  It will NOT auto reconnect when the connection is lost
 * prior to authentication, however.
 *
 * This is a lower level client, meaning the UI is unlikely to
 * use this directly.  Instead, a multiplexer / pub-sub type
 * interface will use this to get and send data.
 */
function RawTextgameProtocolClient() {
    /** PUBLIC ATTRIBUTES **/

    // User function to call when the connection has been established for the
    // first time for a connect() call.
    // No parameters will be provided.
    //
    this.onConnectionEstablished = null;

    // User function to call when the connection has been re-established.
    // No parameters will be provided.
    //
    this.onReconnected = null;

    // User function to call when connection has been (temporarily) lost.
    // No parameters will be provided.
    //
    this.onLostConnection = null;

    // User function to call when permanently disconnected, whether
    // due to an error or disconnect() being called by the user.
    // No parameters will be provided.
    // If an error condition, this will be called first, followed by
    // onError.
    //
    this.onDisconnection = null;

    // User function to call when an error has occurred.  The connection
    // state will revert to disconnected.
    // Note this has nothing to do with errors sent in relation to
    // Channels.
    // An argument with the error in plaintext will be provided.
    //
    this.onError = null;

    // User function to call when a Channel's status has changed (open, closed,
    // blocked, etc).
    // An argument with the details of the status change will be provided.
    //
    this.onChannelStatusChange = null;

    // User function to call when a Text Channel has sent data to us.
    // The first argument will be the channel ID, and the second argument
    // will be the ExternalTextLine.
    //
    this.onTextChannelData = null;

    // User function to call when a non-Text (data) Channel has sent data to
    // us.
    // The first argument will be the channel ID, and the second argument
    // will be the data.
    //
    this.onChannelData = null;

    // User function to call when authentication has succeeded.
    // No parameters will be provided.
    //
    this.onAuthenticationSuccess = null;

    // User function to call when authentication has failed.
    // No parameters will be provided.
    //
    this.onAuthenticationFail = null;

    // User function to call when a site list (and associated details) has
    // been provided by the server.
    // An argument with an array of site details will be provided.
    //
    this.onSiteList = null;


    /** PUBLIC FUNCTIONS **/

    /**
     * @public
     * Called before any other method in this class, this is used when
     * the library is used inside of nodejs instead of the browser.
     * The provider is a browser websocket-like implementation.  Do not
     * call if using in the browser.
     * @param provider A browser-style websocket implementation used while
     * in nodejs.
     */
    this.setWebSocketProvider = function(provider) {
        webSocketProvider = provider;
    };

    /**
     * @public
     * Connects the websocket to the provided URL, if not already connected.
     * The connection will complete in the background.
     * @param {string} url The websocket URL to connect to.
     * @return {boolean} True if connection has successfully started or
     * already connected.
     */
    this.connect = function(url) {
        if (currentState === StateEnum.DISCONNECTED) {
            socketUrl = url;

            if (this.establishConnection()) {
                currentState = StateEnum.CONNECTING;
            } else {
                return false;
            }
        }

        return true;
    };

    /**
     * @public
     * Disconnects from the server.  All queued data is lost.
     */
    this.disconnect = function() {
        cancelTimer();

        if (currentState !== StateEnum.DISCONNECTED) {
            if (currentState === StateEnum.CONNECTED) {
                outgoingControls.push(
                    this.makeClientMessage("Disconnect"));
                this.timerSendPendingData();
            }

            currentState = StateEnum.DISCONNECTED;
            closeSocket();
            this.callDisconnection();
        }

        outgoingData = [];
        outgoingSentData = [];
        outgoingControls = [];

        needAck = false;
        outgoingSer = 0;
        incomingSer = 0;

        authenticationData.username = "";
        authenticationData.password = "";
        authenticationData.site = -1;
        clientAuthenticated = false;
    };

    /**
     * @public
     * Sends a request to get the current site list information, typically
     * used for login screens to select a site to login to.  This can
     * only be sent when connected.
     * @return {boolean} If request was successfully sent.
     */
    this.requestSiteList = function() {
        switch (currentState) {
            case StateEnum.CONNECTED:
            case StateEnum.CONNECTED_AUTH:
            {
                var requestSites = this.makeClientMessage("RequestSiteList");

                outgoingControls.push(requestSites);
                this.timerSendPendingData();
                return true;
            }

            default:
            {
                return false;
            }
        }
    };

    /**
     * @public
     * Sets authentication data and sends out an authentication request if
     * connected.
     * This can also be used to change authentication data while connected,
     * however it won't do anything until a reconnect occurs.
     * @param {string} username The username.
     * @param {string} password The password for the username.
     * @param {number} site The site ID being connected to.
     */
    this.requestAuthentication = function(username, password, site) {
        authenticationData.username = username;
        authenticationData.password = password;
        authenticationData.site = site;

        this.sendAuthentication();
    };

    /**
     * @public
     * @return {boolean} True if client successfully authenticated, false if
     * not.
     */
    this.isAuthenticated = function() {
        return clientAuthenticated;
    };

    /**
     * @public
     * @return {boolean} True if client is successfully connected.  If the
     * client is in the process of reconnecting, it is still considered
     * connected, as all functions are available.
     */
    this.isConnected = function() {
        switch (currentState) {
            case StateEnum.DISCONNECTED:
            case StateEnum.CONNECTING:
            {
                return false;
            }

            default:
            {
                return true;
            }
        }
    };

    /**
     * @public
     * Makes a basic, non-request-response message of the given type.  No
     * attributes are filled in except for what's required for a ClientMessage.
     * @param {string} messageType The type of message to be created.
     * @return {Object} The created message.
     */
    this.makeClientMessage = function(messageType) {
        return {"messageType" : messageType};
    };

    /**
     * @public
     * Sends data over a Channel.
     * @param {number} channelId The channel ID to send the data on.
     * @param {Object} data The data to send.
     */
    this.sendChannelData = function(channelId, data) {
        if (channelId && data) {
            var message = this.makeClientMessage("ChannelData");

            message.content = data;
            message.channelId = channelId;

            // Pick the next ID; we're using IDs between 1 and window * 2.
            // IDs are sequential and will wrap around.
            //
            ++outgoingSer;

            if (outgoingSer > (outgoingWindow * 2)) {
                outgoingSer = 1;
            }

            message.serialId = outgoingSer;

            // Message has been created; queue for sending.
            //
            outgoingData.push(message);
            this.timerSendPendingData();
        }
    };

    /**
     * @public
     * Sends text data over a text Channel.
     * @param {number} channelId The channel ID to send the text data on.
     * @param {Array} text The ExternalTextLine to send.
     */
    this.sendChannelTextData = function(channelId, text) {
        if (channelId && text) {
            var textMessage = this.makeClientMessage("TextData");
            textMessage.textData = text;
            this.sendChannelData(channelId, textMessage);
        }
    };

    /** PRIVATE ATTRIBUTES **/

    // If not using the browser websocket, this is a reference to
    // a nodejs style provider which should work identically to the
    // browser version, except for the .on() style calls.
    //
    var webSocketProvider = null;

    // Enumeration of all states of this instance
    //
    var StateEnum = {
        // Not connected, not requested to connect
        DISCONNECTED : 1,
        // Initial connection in progress.
        CONNECTING : 2,
        // Connected.  Data is flowing.
        CONNECTED : 3,
        // Connected and authenticated.  Data and Channels are flowing.
        CONNECTED_AUTH : 4,
        // Lost connection, about to auto reconnect
        CONNECTION_LOST : 5,
        // Reconnection in progress
        RECONNECTING : 6,
        // Reauthentication after reconnection in progress
        REAUTHENTICATING : 7
    };

    // Current state of instance
    //
    var currentState = StateEnum.DISCONNECTED;

    // Queue of outgoing (to server) Channel data; these are only sent when
    // fully connected and authenticated.
    //
    var outgoingData = [];

    // Data from outgoingData that has been sent and is awaiting an ACK.
    //
    var outgoingSentData = [];

    // Queue of outgoing (to server) control messages such as for
    // authentication and ACKs; these have priority and can be sent as soon as
    // connected.
    //
    var outgoingControls = [];

    // The last outgoing serial ID for the non-control data we sent out.
    //
    var outgoingSer = 0;

    // True if an ACK for the current incomingSer needs to be sent out.
    //
    var needAck = false;

    // The last incoming serial ID for non-control data we received.
    //
    var incomingSer = 0;

    // The size (in messages) of the outcoming window (data from us to server),
    //
    var outgoingWindow = 4096;

    // Callbacks that need to be made after thread of control has unwound.
    // These are primarily callbacks to the registered listener.
    //
    var pendingCallbacks = [];

    // Authentication data provided by the user, used for reconnecting
    // TODO: Is it considered better to use a session ID or something?  Or some way of hiding this better?
    //
    var authenticationData = {username: "", password: "", site: -1};

    // True if client has successfully authenticated.
    //
    var clientAuthenticated = false;

    // Reference to the active websocket
    //
    var socket = null;

    // Reference to listener called when a message is received from the
    // websocket
    //
    var wsMessage = null;

    // Reference to listener called when a websocket is open.
    //
    var wsOpen = null;

    // Reference to listener called when a websocket is closed.
    //
    var wsClose = null;

    // The URL we're connecting to
    //
    var socketUrl = "";

    // If a timer is currently running, its ID will be here.
    // Timers can be for semi-delayed ACKs (to reduce traffic), keepalives,
    // or reconnection.
    // This implies only one timer can be active at a time, to keep things
    // simple.  The exception is the listener callback timer, which handles
    // callbacks for the pendingCallbacks variable.
    //
    var activeTimer = -1;


    /** PRIVATE FUNCTIONS **/

    /**
     * @private
     * Queues callback of onError with the error string, if onError is set.
     * @param {string} reason A user-readable string concerning the error.
     */
    this.callError = function(reason) {
        if ((this.onError !== null) && (typeof(this.onError) === "function")) {
            this.queueExecuteCallback(function(client){
                return function(){
                    client.onError(reason);
                }
            }(this));
        }
    };

    /**
     * @private
     * Queues callback of onConnectionEstablished, if it is set.
     */
    this.callConnectionEstablished = function() {
        if ((this.onConnectionEstablished !== null) &&
            (typeof(this.onConnectionEstablished) === "function")) {
            this.queueExecuteCallback(function(client){
                return function(){
                    client.onConnectionEstablished();
                }
            }(this));
        }
    };

    /**
     * @private
     * Queues callback of onLostConnection, if it is set.
     */
    this.callLostConnection = function() {
        if ((this.onLostConnection !== null) &&
            (typeof(this.onLostConnection) === "function")) {
            this.queueExecuteCallback(function(client){
                return function(){
                    client.onLostConnection();
                }
            }(this));
        }
    };

    /**
     * @private
     * Queues callback of onReconnected, if it is set.
     */
    this.callReconnected = function() {
        if ((this.onReconnected !== null) &&
            (typeof(this.onReconnected) === "function")) {
            this.queueExecuteCallback(function(client){
                return function(){
                    client.onReconnected();
                }
            }(this));
         }
    };

    /**
     * @private
     * Queues callback of onDisconnection, if it is set.
     */
    this.callDisconnection = function() {
        if ((this.onDisconnection !== null) &&
            (typeof(this.onDisconnection) === "function")) {
            this.queueExecuteCallback(function(client){
                return function(){
                    client.onDisconnection();
                }
            }(this));
        }
    };

    /**
     * @private
     * Queues callback of onAuthenticationSuccess, if it is set.
     */
    this.callAuthenticationSuccess = function() {
        if ((this.onAuthenticationSuccess !== null) &&
            (typeof(this.onAuthenticationSuccess) === "function")) {
            this.queueExecuteCallback(function(client){
                return function(){
                    client.onAuthenticationSuccess();
                }
            }(this));
        }
    };

    /**
     * @private
     * Queues callback of onAuthenticationFail, if it is set.
     */
    this.callAuthenticationFail = function() {
        if ((this.onAuthenticationFail !== null) &&
            (typeof(this.onAuthenticationFail) === "function")) {
            this.queueExecuteCallback(function(client){
                return function(){
                    client.onAuthenticationFail();
                }
            }(this));
        }
    };

    /**
     * @private
     * Queues callback of onSiteList, if it is set
     * @param sitelist An array with site information.
     */
    this.callSiteList = function(sitelist) {
        if ((this.onSiteList !== null) &&
            (typeof(this.onSiteList) === "function")) {
            this.queueExecuteCallback(function(client){
                return function(){
                    client.onSiteList(sitelist);
                }
            }(this));
        }
    };

    /**
     * @private
     * Queues callback of onChannelStatusChange, if it is set.
     * @param {object} status The channel status.
     */
    this.callChannelStatusChange = function(status) {
        if ((this.onChannelStatusChange !== null) &&
            (typeof(this.onChannelStatusChange) === "function")) {
            this.queueExecuteCallback(function(client){
                return function(){
                    client.onChannelStatusChange(status);
                }
            }(this));
        }
    };

    /**
     * @private
     * Queues callback of onTextChannelData, if it is set.
     * @param {number} channelId The Channel ID.
     * @param {object} textdata The ExternalTextLine.
     */
    this.callTextChannelData = function(channelId, textdata) {
        if ((this.onTextChannelData !== null) &&
            (typeof(this.onTextChannelData) === "function")) {
            this.queueExecuteCallback(function(client){
                return function(){
                    client.onTextChannelData(channelId, textdata);
                }
            }(this));
        }
    };

    /**
     * @private
     * Queues callback of onChannelData, if it is set.
     * @param {number} channelId The Channel ID.
     * @param {object} data The channel data.
     */
    this.callChannelData = function(channelId, data) {
        if ((this.onChannelData !== null) &&
            (typeof(this.onChannelData) === "function")) {
            this.queueExecuteCallback(function(client){
                return function(){
                    client.onChannelData(channelId, data);
                }
            }(this));
        }
    };

    /**
     * @private
     * If in the correct state, sends an (re)authentication request based on
     * what's in authenticationData.
     */
    this.sendAuthentication = function() {
        // Only allow authentication to be sent when expected.
        //
        switch (currentState) {
            case StateEnum.CONNECTED:
            case StateEnum.REAUTHENTICATING:
            {
                var authMessage = this.makeClientMessage(
                    "AuthenticateRequest");
                authMessage.username = authenticationData.username;
                authMessage.password = authenticationData.password;
                authMessage.site = authenticationData.site;
                authMessage.isReconnect =
                    (currentState === StateEnum.REAUTHENTICATING);
                authMessage.windowSize = outgoingWindow;

                // Send it out as a control message
                outgoingControls.push(JSON.stringify(authMessage));
                this.timerSendPendingData();
            }
        }
    };

    /**
     * @private
     * Handles callback from WebSocket when connection has been (re)established.
     */
    this.processWebsocketConnected = function() {
        cancelTimer();

        // Delay slightly before sending/receiving data due to apparent
        // race conditions in some browsers.  Maybe this isn't really needed.
        //
        activeTimer = setTimeout(function(client) {
            return function(){
                client.timerConnectionOpen();
            }
        }(this), 250);
    };

    /**
     * @private
     * Handles callback from WebSocket when connection is abruptly closed.
     */
    this.processWebsocketClose = function() {
        cancelTimer();

        switch (currentState) {
            case StateEnum.CONNECTING:
            {
                // Failed initial connect.  Error out.
                //
                this.disconnect();
                this.callError(
                    "Unable to establish initial connection.");
                break;
            }

            case StateEnum.CONNECTED_AUTH:
            case StateEnum.RECONNECTING:
            case StateEnum.REAUTHENTICATING:
            {
                var wasReconnecting = (currentState === StateEnum.RECONNECTING) ||
                    (currentState === StateEnum.REAUTHENTICATING);

                // Lost connection after initial connection established
                // and authenticated.  Try and reconnect.
                //
                currentState = StateEnum.CONNECTION_LOST;

                if (! wasReconnecting) {
                    this.callLostConnection();
                }

                outgoingControls = [];

                socket = null;

                activeTimer = setTimeout(function(client) {
                    return function(){
                        client.timerReconnect();
                    }
                }(this), 2000);

                break;
            }
        }
    };

    /**
     * @private
     * Processes the raw text from the websocket.
     * @param rawLine {String} The raw data.
     */
    this.processWebsocketData = function(rawLine) {
        var parsedLine = JSON.parse(rawLine);
        var messageArray;

        if (Array.isArray(parsedLine)) {
            messageArray = parsedLine;
        } else {
            messageArray = [parsedLine];
        }

        // The parsed message is the equivalent of a ClientMessage in C++;
        // look at the type to determine how to process it.
        //
        for (var index = 0; index < messageArray.length; ++index) {
            var message = messageArray[index];

            if (message.messageType) {
                switch (message.messageType) {
                    // Site list
                    case "SiteList":
                        this.callSiteList(message.sites);
                        break;

                    // ACK of data we sent
                    case "DataAcknowledge":
                        // Find the outgoing message corresponding to the ser
                        // ID, and delete everything before and including it.
                        // All our outgoing data that can be ACKed is channel
                        // data.
                        //
                        processServerAck(message.messageSerId);
                        break;

                    // Reconnect ACK
                    case "DataAcknowledgeReconnect":
                        // This is sent to us after we re-authenticate, to tell
                        // us what the server last saw.  After we process this,
                        // message, we must send a similar message back to
                        // initiate data flow.
                        //
                        // Look through outgoingSentData, chop out anything
                        // it saw, and insert the rest back into outgoingData
                        // and resend.
                        //
                        processServerAck(message.messageSerId);
                        outgoingData = outgoingSentData.concat(outgoingData);
                        // outgoingControls
                        var clientAck = this.makeClientMessage(
                            "DataAcknowledgeReconnect");
                        clientAck.messageSerId = incomingSer;
                        outgoingControls.push(clientAck);
                        this.timerSendPendingData();
                        this.callReconnected();
                        break;

                    // Result of trying to authenticate
                    case "AuthenticateResult":
                        if (! message.negotiationResult) {
                            // We failed negotiation; this means our window
                            // is wrong, incompatible version, etc.
                            this.callError(
                                "Failed protocol negotiation.");
                            this.disconnect();
                        } else {
                            if (message.authenticationResult) {
                                if (! clientAuthenticated) {
                                    clientAuthenticated = true;
                                    this.callAuthenticationSuccess();
                                    this.timerSendKeepalive();
                                }
                            } else {
                                if (clientAuthenticated) {
                                    // Client changed password.  We cannot
                                    // reconnect.
                                    this.callError(
                                        "Authentication data changed; "
                                          + "cannot reconnect.");
                                    this.disconnect();
                                } else {
                                    this.callAuthenticationFail();
                                }
                            }
                        }
                        break;

                    // Server telling us to disconnect.
                    case "Disconnect":
                        this.callDisconnection();
                        currentState = StateEnum.DISCONNECTED;
                        this.disconnect();
                        break;

                    // Process channel status messages and channel data
                    case "ChannelData":
                        // Make sure we send an ACK back
                        incomingSer = message.serialId;
                        needAck = true;
                        this.timerSendPendingData();

                        switch (message.content.messageType) {
                            case "ChannelStatusChange":
                                this.callChannelStatusChange(message.content);
                                break;

                            case "TextData":
                                this.callTextChannelData(
                                    message.channelId,
                                    message.content.textData);
                                break;

                            default:
                                this.callChannelData(
                                    message.channelId,
                                    message.content);
                                break;
                        }
                        break;
                }
            }
        }
    };

    /**
     * @private
     * Sends all pending outgoing data (including ACK and control data) to the
     * server after a delay.
     */
    this.timerSendPendingData = function() {
        cancelTimer();

        activeTimer = setTimeout(function(client) {
            return function(){
                // Insert ACK into controls, if needed.
                //
                if (needAck) {
                    var ack = client.makeClientMessage("DataAcknowledge");
                    ack.messageSerId = incomingSer;

                    outgoingControls.push(ack);
                    needAck = false;
                }

                // Send controls
                //
                for (var cIndex = 0; cIndex < outgoingControls.length;
                     ++cIndex) {
                    sendTextData(JSON.stringify(outgoingControls[cIndex]));
                }

                outgoingControls = [];

                // Send outgoingData up to the window.
                //
                var remain = outgoingWindow - outgoingSentData.length;
                var index = -1;
                if (outgoingData.length && remain) {
                    for (index = 0; (index < outgoingData.length) &&
                        remain; ++index) {
                        sendTextData(JSON.stringify(outgoingData[index]));
                        --remain;
                    }
                }

                if (index !== -1) {
                    // We sent something, so put the relevant messages into
                    // outgoingSentData.
                    //
                    var sent = outgoingData.slice(0, index);
                    outgoingData.splice(0, index);
                    outgoingSentData = outgoingSentData.concat(sent);
                }

                // Re-enable keepalive since we're done sending.
                client.timerSendKeepalive();
            }
        }(this), 50);
    };

    /**
     * @private
     * Handles, after the timer delay, anything needed to set up the
     * connection.  It will also handle re-authentication after a reconnect.
     */
    this.timerConnectionOpen = function() {
        cancelTimer();

        if (currentState === StateEnum.CONNECTING) {
            // First connection.  We're ready to authenticate or get a site
            // list.
            //
            currentState = StateEnum.CONNECTED;
            this.callConnectionEstablished();
        } else if (currentState === StateEnum.RECONNECTING) {
            // Reconnection.  We can send a stored authentication immediately.
            //
            currentState = StateEnum.REAUTHENTICATING;

            this.sendAuthentication();

            activeTimer = setTimeout(function(client) {
                return function(){
                    client.timerReconnectWatchdog();
                }
            }(this), 5000);
        }
    };

    /**
     * @private
     * Creates a websocket, adds the required listeners, then initiates a
     * connection.
     * @return {boolean} True if success, false if error.
     */
    this.establishConnection = function() {
        try {
            wsMessage = function(client) {
                return function(socketData){
                    client.processWebsocketData(socketData.data);
                };
            }(this);
            wsOpen = function(client) {
                return function(){
                    client.processWebsocketConnected();
                };
            }(this);
            wsClose = function(client) {
                return function(){
                    client.processWebsocketClose();
                };
            }(this);

            if (webSocketProvider) {
                socket = new webSocketProvider(
                    socketUrl,
                    []);

                socket.on('message', wsMessage);
                socket.on('open', wsOpen);
                socket.on('close', wsClose);
            } else {
                socket = new WebSocket(socketUrl, []);

                socket.onmessage = wsMessage;
                socket.onopen = wsOpen;
                socket.onclose = wsClose;
            }
        } catch (e) {
            socket = null;
            return false;
        }

        return true;
    };

    /**
     * @private
     * Closes the socket and nulls it out without any callbacks.
     */
    var closeSocket = function() {
        if (socket !== null) {
            try {
                if (webSocketProvider) {
                    socket.removeListener('open', wsOpen);
                    socket.removeListener('message', wsMessage);
                    socket.removeListener('close', wsClose);
                } else {
                    socket.onclose = null;
                    socket.onmessage = null;
                    socket.onopen = null;
                }

                socket.close();
            } catch (e) {
                // Do nothing, just ignore.
            }
            socket = null;
        }
    };

    /**
     * @private
     * Sends a keepalive message, and keeps sending it on a timer until the
     * timer is cancelled.
     */
    this.timerSendKeepalive = function() {
        cancelTimer();

        if (this.isConnected()) {
            activeTimer = setTimeout(function(client) {
                return function(){
                    sendTextData("KeepAlive");
                    client.timerSendKeepalive();
                }
            }(this), 10000);
        }
    };

    /**
     * @private
     * Proactively re-connects if the socket doesn't establish a connection
     * in a reasonable amount of time.  The OS/Browser level timeouts are
     * way too long in most cases.
     */
    this.timerReconnectWatchdog = function() {
        switch (currentState) {
            case StateEnum.RECONNECTING:
            case StateEnum.REAUTHENTICATING:
            {
                // The connection did not establish in a timely manner.
                // Close it and retry the connect.  If we don't do this,
                // it could be a very long time before the OS/browser
                // times it out.
                //
                closeSocket();
                currentState = StateEnum.CONNECTION_LOST;
                outgoingControls = [];

                this.timerReconnect();
            }
        }
    };

    /**
     * @private
     * Periodically tries to reconnect, via a timer.
     */
    this.timerReconnect = function() {
        this.establishConnection();

        currentState = StateEnum.RECONNECTING;

        cancelTimer();
        activeTimer = setTimeout(function(client) {
            return function(){
                client.timerReconnectWatchdog();
            }
        }(this), 5000);
    };

    /**
     * @private
     * Queues the given callback, and schedules it for execution after the
     * stack unwinds.
     * @param {function} callback The callback to execute when thread of
     * control unwinds.
     */
    this.queueExecuteCallback = function(callback) {
        // The callback timer is considered running if the array already has
        // something in it.
        //
        if (pendingCallbacks.length === 0) {
            // Not currently running.  Schedule to run after thread unwinds.
            setTimeout(function(client) {
                return function() {
                    for (var index = 0; index < pendingCallbacks.length;
                         ++index) {
                        pendingCallbacks[index]();
                    }

                    pendingCallbacks = [];
                }
            }(this), 0);
        }

        pendingCallbacks.push(callback);
    };

    /**
     * @private
     * Processes an ACK (acknowledge) from the server by removing any ACKed
     * messages from outgoingSentData.  The ID being ACKed is inclusive; that
     * is, any IDs of messages sent before it are automatically considered as
     * ACKed as well.
     * @param {number} serId The serial ID being ACKed.
     */
    var processServerAck = function(serId) {
        var foundIndex = -1;

        for (var index = 0; index < outgoingSentData.length;
             ++index) {
            if (outgoingSentData[index].serialId === serId) {
                foundIndex = index;
                break;
            }
        }

        if (foundIndex >= 0) {
            outgoingSentData.splice(0, foundIndex + 1);
        }
    };

    /**
     * @private
     * Sends text data across the websocket, catching any exceptions that may
     * occur.
     * @param {string} text The string data to send.
     */
    var sendTextData = function(text) {
        if (socket) {
            try {
                socket.send(text);
            } catch (e) {
                // Ignore.  The 'closed' method will be called by the
                // websocket API soon enough.
            }
        }
    };

    /**
     * @private
     * If a timer is active, cancels it.
     */
    var cancelTimer = function() {
        if (activeTimer !== -1) {
            clearTimeout(activeTimer);
            activeTimer = -1;
        }
    };
}

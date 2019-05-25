// --------------------------------------------------------------------------
/**
 * Base prototype for send and receive Channels.  It contains attributes
 * and functions common to all Channels.
 * @param {number} channelId The ID number of the channel.
 * @param {string} channelStatus The channel status.
 * @param {string} channelName The channel name.
 * @param {string} channelType The channel type.
 * @param {string} channelSubtype The channel subtype.
 * @param {TextgameProtocolClient} clientRef Reference to the client.
 */
function Channel(channelId, channelStatus, channelName, channelType,
                 channelSubtype, clientRef) {

    /** Public attributes and methods **/

    // The ID of the channel
    //
    this.id = channelId;

    // The current channel status
    // @see ChannelStatusEnum
    //
    this.status = this.convertChannelStatus(channelStatus);

    // The name of the channel.
    //
    this.name = channelName;

    // The type of the channel.
    // @see ChannelTypeEnum
    //
    this.type = this.convertChannelType(channelType);

    // The subtype of the channel.
    //
    this.subtype = channelSubtype;

    // A reference back to to TextgameProtocolClient.
    //
    this.clientRef = clientRef;

    // If not null, the provided function will be called back when the
    // channel's status changes.
    // The ID and the status will be provided:  onStatusChange(ID, status);
    //
    this.onStatusChange = null;

    /**
     * @public
     * @return {boolean} True if channel is blocked.
     */
    this.isBlocked = function() {
        return (this.status === this.ChannelStatusEnum.BLOCK) ||
            (this.status === this.ChannelStatusEnum.OPEN);
    };

    /**
     * @public
     * @return {boolean} True if channel is open (may be blocked).
     */
    this.isOpen = function() {
        return this.status !== this.ChannelStatusEnum.CLOSE;
    };

    var incoming = false;
}

// Enumeration of all the states a channel can have.
//
Channel.prototype.ChannelStatusEnum = {
    // Channel was just opened.  It is blocked.
    OPEN : 'open',
    // Channel is permanently closed.
    CLOSE : 'close',
    // Channel is blocked.
    BLOCK : 'block',
    // Channel is unblocked.
    UNBLOCK : 'unblock',
    UNKNOWN : ''
};

// Enumeration of all the types a channel can be.
//
Channel.prototype.ChannelTypeEnum = {
    TEXT : 'text',
    DATA : 'data',
    UNKNOWN : ''
};

/**
 * @public
 * @param {string} status The channel status as a string.
 * @return {ChannelStatusEnum} Channel status as an enum.
 */
Channel.prototype.convertChannelStatus = function(status) {
    for (var val in this.ChannelStatusEnum) {
        var enumVal = this.ChannelStatusEnum[val];

        if (enumVal === status) {
            return enumVal;
        }
    }

    return this.ChannelStatusEnum.UNKNOWN;
};

/**
 * @package
 * Sets a new status for the Channel, calling the status change listener
 * if it actually changed.
 * @param {string} status The Channel's new status.
 */
Channel.prototype.updateChannelStatus = function(status) {
    var newStatus = this.convertChannelStatus(status);

    if (newStatus !== this.status) {
        this.status = newStatus;
        this.callStatusChange();
    }
};

/**
 * @public
 * @param {string} type The channel type as a string.
 * @return {ChannelTypeEnum} Channel type as an enum.
 */
Channel.prototype.convertChannelType = function(type) {
    for (var val in this.ChannelTypeEnum) {
        var enumVal = this.ChannelTypeEnum[val];

        if (enumVal === type) {
            return enumVal;
        }
    }

    return this.ChannelTypeEnum.UNKNOWN;
};

/**
 * @package
 * Calls a status change listener with the current status,
 */
Channel.prototype.callStatusChange = function() {
    if ((this.onStatusChange !== null) &&
        (typeof(this.onStatusChange) === "function")) {
        this.onStatusChange(this.id, this.status);
    }
};


// --------------------------------------------------------------------------
/**
 * Creates a Channel for sending data to the server.
 * Inherits from Channel.
 * @param {number} channelId The ID number of the channel.
 * @param {string} channelStatus The channel status.
 * @param {string} channelName The channel name.
 * @param {string} channelType The channel type.
 * @param {string} channelSubtype The channel subtype.
 * @param {TextgameProtocolClient} clientRef Reference to the client.
 */
function SendChannel(channelId, channelStatus, channelName, channelType,
    channelSubtype, clientRef) {
    // Used to initialize parent
    Channel.call(this, channelId, channelStatus, channelName, channelType,
        channelSubtype, clientRef);

    /** Public attributes and methods **/
    // See below
}


// --------------------------------------------------------------------------
function ReceiveChannel(channelId, channelStatus, channelName, channelType,
                        channelSubtype, clientRef) {
    // Used to initialize parent
    Channel.call(this, channelId, channelStatus, channelName, channelType,
        channelSubtype, clientRef);

    // Called when non-text data has been received on the Channel.
    // The ID and the data will be provided:  onReceiveData(ID, data);
    //
    this.onReceiveData = null;

    // Called when external text data has been received on the Channel.
    // The ID and the text data will be provided:
    // onReceiveTextData(ID, text);
    //
    this.onReceiveTextData = null;
}


// Inheritance
//
SendChannel.prototype = Object.create(Channel.prototype);
ReceiveChannel.prototype = Object.create(Channel.prototype);


/**
 * @public
 * Sends a non-text message (ClientMessage) on the channel.
 * @param {Object} data The message to send.
 * @return {boolean} True if able to send.  Note the server may not get it
 * right away if in the middle of reconnecting, etc.
 */
SendChannel.prototype.sendData = function(data) {
    if ((this.type !== this.ChannelTypeEnum.DATA) ||
        (this.status !== this.ChannelStatusEnum.OPEN)) {
        // Cannot send on a blocked or closed channel, or one that is
        // not for data.
        return false;
    }

    this.clientRef.sendData(this, data);
    return true;
};

/**
 * @public
 * Sends an external text message on the channel.
 * @param {Array} text The text to send.
 * @return {boolean} True if able to send.  Note the server may not get it
 * right away if in the middle of reconnecting, etc.
 */
SendChannel.prototype.sendText = function(text) {
    if ((this.type !== this.ChannelTypeEnum.TEXT) ||
        (this.status !== this.ChannelStatusEnum.OPEN)) {
        // Cannot send on a blocked or closed channel.
        return false;
    }

    this.clientRef.sendText(this, text);
    return true;
};


/**
 * @package
 * Calls the non-text data listener with received data.
 * @param {Object} data The received data to give to the listener.
 */
ReceiveChannel.prototype.callReceiveData = function(data) {
    if ((this.onReceiveData !== null) &&
        (typeof(this.onReceiveData) === "function")) {
        this.onReceiveData(this.id, data);
    }
};

/**
 * @package
 * Calls the external text data listener with received text data.
 * @param {Array} text The received text data to give to the listener.
 */
ReceiveChannel.prototype.callReceiveTextData = function(text) {
    if ((this.onReceiveTextData !== null) &&
        (typeof(this.onReceiveTextData) === "function")) {
        this.onReceiveTextData(this.id, text);
    }
};


/**
 * UIs can consider this to be their interface to the MUTGOS server.  It
 * provides everything needed to connect, perform authentication, and get
 * data to and from Channels.
 */
function TextgameProtocolClient() {
    /** PUBLIC ATTRIBUTES **/

    // User function to call when the connection has been established for the
    // first time for a connect() call.
    // Set before attempting to connect.
    // No parameters will be provided.
    //
    this.onConnectionEstablished = null;

    // User function to call when the connection has been re-established.
    // Set before attempting to connect.
    // No parameters will be provided.
    //
    this.onReconnected = null;

    // User function to call when connection has been (temporarily) lost.
    // Set before attempting to connect.
    // No parameters will be provided.
    //
    this.onLostConnection = null;

    // User function to call when permanently disconnected, whether
    // due to an error or disconnect() being called by the user.
    // Set before attempting to connect.
    // No parameters will be provided.
    // If an error condition, this will be called first, followed by
    // onError.
    //
    this.onDisconnection = null;

    // User function to call when an error has occurred.  The connection
    // state will revert to disconnected.
    // Note this has nothing to do with errors sent in relation to
    // Channels.
    // Set before attempting to connect.
    // An argument with the error in plaintext will be provided.
    //
    this.onError = null;

    // User function to call when authentication has succeeded.
    // Set before attempting to connect.
    // No parameters will be provided.
    //
    this.onAuthenticationSuccess = null;

    // User function to call when authentication has failed.
    // No parameters will be provided.
    //
    this.onAuthenticationFail = null;

    // User function to call when a site list (and associated details) has
    // been provided by the server.
    // Set before attempting to connect.
    // An argument with an array of site details will be provided.
    //
    this.onSiteList = null;

    // User function to call when a channel has been opened (send or receive).
    // Set before attempting to connect.
    // An argument with the Channel (see above) will be provided.
    //
    this.onChannelOpen = null;

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
        rawClient.setWebSocketProvider(provider);
    };

    /**
     * @public
     * Makes a basic, non-request-response message of the given type.  No
     * attributes are filled in except for what's required for a ClientMessage.
     * @param {string} messageType The type of message to be created.
     * @return {Object} The created message.
     */
    this.makeClientMessage = function(messageType) {
        return rawClient.makeClientMessage(messageType);
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
        if (! connectCalled) {
            // Pass the direct callbacks to the raw client
            //
            rawClient.onConnectionEstablished = this.onConnectionEstablished;
            rawClient.onReconnected = this.onReconnected;
            rawClient.onLostConnection = this.onLostConnection;
            rawClient.onDisconnection = this.processDisconnection;
            rawClient.onError = this.onError;
            rawClient.onAuthenticationSuccess = this.onAuthenticationSuccess;
            rawClient.onAuthenticationFail = this.onAuthenticationFail;
            rawClient.onSiteList = this.onSiteList;

            rawClient.onChannelStatusChange = this.updatedChannelStatus;
            rawClient.onChannelData = this.receiveChannelData;
            rawClient.onTextChannelData = this.receiveChannelTextData;

            if (rawClient.connect(url)) {
                connectCalled = true;
                return true;
            } else {
                return false;
            }
        }

        return true;
    };

    /**
     * @public
     * @return {boolean} True if client is successfully connected.  If the
     * client is in the process of reconnecting, it is still considered
     * connected, as all functions are available.
     */
    this.isConnected = function() {
        return rawClient.isConnected();
    };

    /**
     * @public
     * Disconnects from the server.  All queued data is lost.
     */
    this.disconnect = function() {
        rawClient.disconnect();
        this.closeAllChannels();
        connectCalled = false;
    };

    /**
     * @public
     * Sends a request to get the current site list information, typically
     * used for login screens to select a site to login to.  This can
     * only be sent when connected.
     * @return {boolean} If request was successfully sent.
     */
    this.requestSiteList = function() {
        return rawClient.requestSiteList();
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
    this.authenticate = function(username, password, site) {
        rawClient.requestAuthentication(username, password, site);
    };

    /**
     * @public
     * @return {boolean} True if client successfully authenticated, false if
     * not.
     */
    this.isAuthenticated = function() {
        return rawClient.isAuthenticated();
    };

    /** PRIVATE ATTRIBUTES **/

    // True if connect() has been called, to prevent it from being called
    // again.
    //
    var connectCalled = false;

    // Maps channel ID to its container.  Only valid channels will
    // be present.
    //
    var channelMap = {};

    // Reference to the raw client, which operates at the protocol level.
    //
    var rawClient = new RawTextgameProtocolClient();

    // Used for callback methods so they have correct this
    //
    var myInstance = this;

    /** PRIVATE FUNCTIONS **/

    /**
     * @package
     * Called by RawTextgameProtocolClient when a Channel is opened, the status
     * changes, or is closed.  The Channel will be added or removed here, as
     * appropriate, and listeners will be called.
     * @param {Object} status The Channel status.
     */
    this.updatedChannelStatus = function(status) {
        if (status.channelStatus === "open") {
            // Newly opened Channel; we need to create the Channel, add it to
            // the map, and call the designated 'new channel listener'.
            //
            var newChannel;

            if (status.channelOut) {
                newChannel = new ReceiveChannel(
                    status.channelId,
                    status.channelStatus,
                    status.channelName,
                    status.channelType,
                    status.channelSubtype,
                    myInstance);
            } else {
                newChannel = new SendChannel(
                    status.channelId,
                    status.channelStatus,
                    status.channelName,
                    status.channelType,
                    status.channelSubtype,
                    myInstance);
            }

            channelMap[status.channelId.toString()] = newChannel;

            if ((myInstance.onChannelOpen !== null) &&
                (typeof(myInstance.onChannelOpen) === "function")) {
                myInstance.onChannelOpen(newChannel);
            }
        } else if (status.channelStatus === "close") {
            // Channel is closing.  Update status on Channel, call listener,
            // and remove from map as the Channel is now invalid.
            //
            var existingChannel = channelMap[status.channelId.toString()];

            if (existingChannel) {
                existingChannel.updateChannelStatus(status.channelStatus);
                delete channelMap[status.channelId.toString()];
            }
        } else {
            // Standard Channel status update.
            //
            var existingChannel = channelMap[status.channelId.toString()];

            if (existingChannel) {
                existingChannel.updateChannelStatus(status.channelStatus);
            }
        }
    };

    /**
     * @package
     * Processes a disconnection callback from the raw client, cleaning up
     * stuff here after calling the listener.
     */
    this.processDisconnection = function() {
        if ((myInstance.onDisconnection !== null) &&
            (typeof(myInstance.onDisconnection) === "function")) {
            myInstance.onDisconnection();
        }

        myInstance.closeAllChannels();
        connectCalled = false;
    };

    /**
     * @package
     * Marks all Channels as closed and removes them.
     */
    this.closeAllChannels = function() {
        for (var channelId in channelMap) {
            if (channelMap.hasOwnProperty(channelId)) {
                var channel = channelMap[channelId];
                channel.updateChannelStatus(channel.ChannelStatusEnum.CLOSE);
            }
        }

        channelMap = {};
    };

    /**
     * @package
     * Called by RawTextgameProtocolClient when a Channel has received external
     * text data.  This will pass it on to the appropriate listener.
     * @param {number} id The channel ID.
     * @param {Array} text The external text data.
     */
    this.receiveChannelTextData = function(id, text) {
        var existingChannel = channelMap[id.toString()];

        if (existingChannel) {
            existingChannel.callReceiveTextData(text);
        }
    };

    /**
     * @package
     * Called by RawTextgameProtocolClient when a Channel has received external
     * non-text data.  This will pass it on to the appropriate listener.
     * @param {number} id The channel ID.
     * @param {Object} data The data.
     */
    this.receiveChannelData = function(id, data) {
        var existingChannel = channelMap[id.toString()];

        if (existingChannel) {
            existingChannel.callReceiveData(data);
        }
    };

    /**
     * @package
     * Not for direct use by clients.
     * Sends a non-text message (ClientMessage).
     * Note the data may not be sent right away if in the middle of a
     * reconnect, etc.
     * @param {SendChannel} sendChannel The SendChannel the data is to be
     * sent on.
     * @param {Object} data The data to send.
     */
    this.sendData = function(sendChannel, data) {
        rawClient.sendChannelData(sendChannel.id, data);
    };

    /**
     * @package
     * Not for direct use by clients.
     * Sends an external text message on the channel.
     * Note the data may not be sent right away if in the middle of a
     * reconnect, etc.
     * @param {SendChannel} sendChannel The SendChannel the text data is to be
     * sent on.
     * @param {Array} text The external text to send.
     */
    this.sendText = function(sendChannel, text) {
        // Remove any attributes that aren't needed due to being false, etc.
        //
        for (var index = 0; index < text.length; ++index) {
            var entry = text[index];

            if (! entry.bold) {
                delete entry.bold;
            }

            if (! entry.italic) {
                delete entry.italic;
            }

            if (! entry.underline) {
                delete entry.underline;
            }

            if (! entry.inverse) {
                delete entry.inverse;
            }

            if (entry.color) {
                if (entry.color !== entry.ColorEnum.CUSTOM) {
                    // Only custom colors need RGB.
                    //
                    delete entry.red;
                    delete entry.green;
                    delete entry.blue;
                }
            }
        }

        rawClient.sendChannelTextData(sendChannel.id, text);
    };
}

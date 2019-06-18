// --------------------------------------------------------------------------
/**
 * Represents an ID for an Entity.  Generally used for sending, but is a
 * perfect guide to how a received Entity ID will look.
 * @param {number} siteId Site ID.
 * @param {number} entityId Entity ID.
 */
function EntityId(siteId, entityId) {
    this.siteId = siteId;
    this.entityId = entityId;
}

// --------------------------------------------------------------------------
/**
 * Base prototype for all external text.  Generally used for sending,
 * but is a perfect guide to how received text will look.
 */
function ExternalText(type) {
    /** Public attributes and methods **/

    // The type of external text this is.
    this.textType = this.convertTextType(type);
}

// --------------------------------------------------------------------------
/**
 * Represents plain (unformatted) text.  Inherits from ExternalText.
 * @param {string} text The unformatted text.
 */
function ExternalPlainText(text) {
    // Used to initialize parent
    ExternalText.call(this, this.TextTypeEnum.PLAIN);

    /** Public attributes and methods **/

    // The unformatted text.
    //
    this.plainText = text;
}

// --------------------------------------------------------------------------
/**
 * Represents formatted text.  Inherits from ExternalPlainText.
 * @param {string} text The actual text to store.  Formatting is specified
 * after construction.
 */
function ExternalFormattedText(text) {
    // Used to initialize parent
    //
    ExternalPlainText.call(this, text);
    this.textType = this.TextTypeEnum.FORMATTED;

    /** Public attributes and methods **/

    // Pre-defined color to use for text, or default for client's default, or
    // custom to use manual RGB values.
    //
    this.color = this.ColorEnum.DEFAULT;

    // Custom red value, 0 through 255
    //
    this.red = 0;

    // Custom green value, 0 through 255
    //
    this.green = 0;

    // Custom blue value, 0 through 255
    //
    this.blue = 0;

    // Bold text
    //
    this.bold = false;

    // Italic text
    //
    this.italic = false;

    // Underline text
    //
    this.underline = false;

    // Inverse text
    //
    this.inverse = false;
}

// --------------------------------------------------------------------------
/**
 * Represents an external text that has an Entity 'link' in it.
 * @param {EntityId} id The Entity ID.
 * @param {string} name The displayed name for the Entity.
 * @param {IdTypeEnum} type The type of Entity ID being displayed.
 */
function ExternalIdText(id, name, type) {
    // Used to initialize parent
    ExternalText.call(this, this.TextTypeEnum.ID);

    // The Entity ID associated with this text.
    //
    this.id = id;

    // The name to be displayed for the Entity.
    //
    this.name = name;

    // The type of ID text
    //
    this.type = type;
}

// --------------------------------------------------------------------------
/**
 * Represents a URL in external text, with details about how to render it.
 * @param {string} url The URL.
 * @param {string} name The name/description/caption of the URL.
 * @param {UrlTypeEnum} type The type of the URL to aid in proper rendering.
 */
function ExternalUrlText(url, name, type) {
    // Used to initialize parent
    ExternalText.call(this, this.TextTypeEnum.URL);

    // The URL to be displayed
    //
    this.url = url;

    // The name (caption/description) for the URL.
    //
    this.name = name;

    // The type of URL to aid in rendering it
    //
    this.type = type;
}



// --------------------------------------------------------------------------
// Inheritance
//
ExternalPlainText.prototype = Object.create(ExternalText.prototype);
ExternalFormattedText.prototype = Object.create(ExternalPlainText.prototype);
ExternalIdText.prototype = Object.create(ExternalText.prototype);
ExternalUrlText.prototype = Object.create(ExternalText.prototype);



/**
 * Outputs a more compact form of the Entity ID.
 * @return {string} The Entity ID as a string
 */
EntityId.prototype.toString = function() {
    return "#" + this.siteId + "-" + this.entityId;
};

/**
 * @param {boolean} withSite True to include site ID in output.
 * @return {string} The entity ID as a user-readable string.
 */
EntityId.prototype.asString = function(withSite) {
    var id = "#";

    if (withSite) {
        id += this.siteId + "-";
    }

    id += this.entityId;

    return id;
};

// Enumerates all the different text types.
//
ExternalText.prototype.TextTypeEnum = {
    PLAIN : 'plain',
    FORMATTED : 'formatted',
    URL : 'url',
    ID : 'id',
    UNKNOWN : ''
};

/**
 * @public
 * @param {string} type The text type as a string.
 * @return {TextTypeEnum} Text type as an enum.
 */
ExternalText.prototype.convertTextType = function(type) {
    for (var val in this.TextTypeEnum) {
        var enumVal = this.TextTypeEnum[val];

        if (enumVal === type) {
            return enumVal;
        }
    }

    return this.TextTypeEnum.UNKNOWN;
};

// Enumerates all the different text colors.  Default is client's default
// color, and custom allows the RGB values to be manually set.
//
ExternalFormattedText.prototype.ColorEnum = {
    DEFAULT : 'default',
    CUSTOM : 'custom',
    BLACK : 'black',
    RED : 'red',
    YELLOW : 'yellow',
    GREEN : 'green',
    CYAN : 'cyan',
    BLUE : 'blue',
    MAGENTA : 'magenta',
    WHITE : 'white',
    UNKNOWN : ''
};

/**
 * @public
 * @param {string} color The color (as a string) to convert.
 * @return {ColorEnum} Color as an enum.
 */
ExternalFormattedText.prototype.convertColor = function(color) {
    for (var val in this.ColorEnum) {
        var enumVal = this.ColorEnum[val];

        if (enumVal === color) {
            return enumVal;
        }
    }

    return this.ColorEnum.UNKNOWN;
};

// Enumerates all the Entity types that can be represented by ExternalIdText.
//
ExternalIdText.prototype.IdTypeEnum = {
    ENTITY : 'entity',
    ACTION : 'action',
    EXIT : 'exit',
    UNKNOWN : ''
};

/**
 * @public
 * @param {string} idType The ID type as a string.
 * @return {IdTypeEnum} idType as an enum.
 */
ExternalIdText.prototype.convertIdType = function(idType) {
    for (var val in this.IdTypeEnum) {
        var enumVal = this.IdTypeEnum[val];

        if (enumVal === idType) {
            return enumVal;
        }
    }

    return this.IdTypeEnum.UNKNOWN;
};

// Represents the type of URL, to aid in knowing how to render it.
//
ExternalUrlText.prototype.UrlTypeEnum = {
    PAGE : 'page',
    IMAGE : 'image',
    AUDIO : 'audio',
    UNKNOWN : ''
};

/**
 * @public
 * @param {string} urlType The URL type as a string.
 * @return {UrlTypeEnum} urlType as an enum.
 */
ExternalUrlText.prototype.convertUrlType = function(urlType) {
    for (var val in this.UrlTypeEnum) {
        var enumVal = this.UrlTypeEnum[val];

        if (enumVal === urlType) {
            return enumVal;
        }
    }

    return this.UrlTypeEnum.UNKNOWN;
};

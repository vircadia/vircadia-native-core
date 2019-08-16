var customEmojiList = [
    {
        "name": "sam",
        "filename": "sam.png",
        "keywords": [
            "sam",
            "troll"
        ]
    }
]
try {
    if (module) {
        module.exports = customEmojiList;
    }
} catch (e) {
    console.log("error exporting:\n", e);
}
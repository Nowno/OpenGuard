/**
 * Simple log function, I probably shouldn't have wasted time finding emojis, but they don't print in C++, guilty pleasure.
 */
function Log(emoji, type, message)
{
    let current_time = new Date().toLocaleString();

    console.log(`${emoji} [${current_time}] [${type}] ${message}`);
}

export { Log };
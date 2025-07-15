function Log(emoji, type, message)
{

    let current_time = new Date().toLocaleString();

    console.log(`${emoji} [${current_time}] [${type}] ${message}`);
}

export { Log };
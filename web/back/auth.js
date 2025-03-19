import config from "./config_manager.js";

class Auth
{
    static AuthenticateUser(username, password)
    {
        return username === config.ws.username && password === config.ws.password;
    }
}

export default Auth;

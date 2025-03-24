import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: vscode.ExtensionContext) {
    console.log('Activating Goo Language extension');

    const config = vscode.workspace.getConfiguration('goo');
    
    // Get the server path from configuration
    const serverPath = config.get<string>('lsp.path') || 'goo-lsp';
    const serverArgs = config.get<string[]>('lsp.args') || [];
    
    // Add command-line arguments based on configuration
    if (!config.get<boolean>('diagnostics.enabled')) {
        serverArgs.push('--no-diagnostics');
    }
    if (!config.get<boolean>('hover.enabled')) {
        serverArgs.push('--no-hover');
    }
    if (!config.get<boolean>('completion.enabled')) {
        serverArgs.push('--no-completion');
    }
    if (!config.get<boolean>('definition.enabled')) {
        serverArgs.push('--no-definition');
    }
    if (!config.get<boolean>('references.enabled')) {
        serverArgs.push('--no-references');
    }
    if (!config.get<boolean>('formatting.enabled')) {
        serverArgs.push('--no-formatting');
    }
    if (!config.get<boolean>('symbols.enabled')) {
        serverArgs.push('--no-symbols');
    }
    if (!config.get<boolean>('highlight.enabled')) {
        serverArgs.push('--no-highlight');
    }
    if (!config.get<boolean>('rename.enabled')) {
        serverArgs.push('--no-rename');
    }
    if (!config.get<boolean>('signatureHelp.enabled')) {
        serverArgs.push('--no-signature-help');
    }
    
    // Verbose logging if trace is enabled
    const trace = config.get<string>('lsp.trace.server');
    if (trace === 'verbose') {
        serverArgs.push('--verbose');
    }
    
    // Server options - either use the executable or the debug server
    const serverOptions: ServerOptions = {
        run: {
            command: serverPath,
            args: serverArgs,
            options: {
                env: process.env
            }
        },
        debug: {
            command: serverPath,
            args: [...serverArgs, '--verbose'],
            options: {
                env: process.env
            }
        }
    };
    
    // Client options
    const clientOptions: LanguageClientOptions = {
        documentSelector: [
            { scheme: 'file', language: 'goo' },
            { scheme: 'untitled', language: 'goo' }
        ],
        synchronize: {
            // Synchronize the setting section 'goo' to the server
            configurationSection: 'goo',
            fileEvents: [
                vscode.workspace.createFileSystemWatcher('**/*.goo'),
                vscode.workspace.createFileSystemWatcher('**/goo.toml')
            ]
        },
        outputChannelName: 'Goo Language Server',
        traceOutputChannel: vscode.window.createOutputChannel('Goo Language Server Trace'),
    };
    
    // Create and start the client
    client = new LanguageClient(
        'goo-language-server',
        'Goo Language Server',
        serverOptions,
        clientOptions
    );
    
    // Register the restart command
    const restartCommand = vscode.commands.registerCommand('goo.restartLSP', async () => {
        if (client) {
            await client.stop();
            client.start();
            vscode.window.showInformationMessage('Goo Language Server restarted');
        }
    });
    
    context.subscriptions.push(restartCommand);
    
    // Start the client
    client.start();
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
} 
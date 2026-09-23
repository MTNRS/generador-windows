using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

public sealed class MainForm : Form
{
    readonly TextBox project = new TextBox { Name = "ProjectPath" };
    readonly Button browse = new Button { Text = "Examinar…", Name = "Browse" };
    readonly Button generate = new Button { Text = "Generar informe", Name = "Generate" };
    readonly Button open = new Button { Text = "Abrir carpeta del informe", Enabled = false, Name = "OpenOutput" };
    readonly Label status = new Label { Text = "Selecciona la carpeta de tu proyecto para empezar.", AutoSize = false, Name = "Status" };
    readonly TextBox details = new TextBox { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, Name = "Details" };
    string outputFolder;
    bool busy;

    public MainForm()
    {
        Text = "Generador de informes · Jocarsa";
        Font = new Font("Segoe UI", 10);
        ClientSize = new Size(660, 390);
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        AutoScaleMode = AutoScaleMode.Dpi;
        var title = new Label { Text = "Genera el informe de tu proyecto", AutoSize = true, Font = new Font("Segoe UI", 16, FontStyle.Bold), Location = new Point(24, 20) };
        var label = new Label { Text = "Carpeta del proyecto", AutoSize = true, Location = new Point(24, 70) };
        project.SetBounds(24, 98, 490, 28);
        browse.SetBounds(526, 96, 110, 32);
        var hint = new Label { Text = "El informe se guardará en la subcarpeta documentacion del proyecto.", AutoSize = true, Location = new Point(24, 142) };
        generate.SetBounds(24, 179, 170, 38);
        open.SetBounds(208, 179, 220, 38);
        status.SetBounds(24, 233, 612, 44);
        details.SetBounds(24, 284, 612, 82);
        Controls.AddRange(new Control[] { title, label, project, browse, hint, generate, open, status, details });
        AcceptButton = generate;
        browse.Click += Browse;
        generate.Click += async (sender, args) => await Generate();
        project.TextChanged += (sender, args) => { open.Enabled = false; };
        open.Click += (sender, args) => {
            try { Process.Start(new ProcessStartInfo(outputFolder) { UseShellExecute = true }); }
            catch (Exception ex) { status.Text = "No se pudo abrir la carpeta."; details.Text = ex.Message; }
        };
        FormClosing += (sender, args) => {
            if (busy) { args.Cancel = true; status.Text = "Espera a que termine la generación antes de cerrar."; }
        };
    }

    void Browse(object sender, EventArgs args)
    {
        using (var dialog = new FolderBrowserDialog())
        {
            dialog.Description = "Selecciona la carpeta del proyecto que quieres documentar";
            dialog.ShowNewFolderButton = false;
            if (Directory.Exists(project.Text)) dialog.SelectedPath = project.Text;
            if (dialog.ShowDialog(this) == DialogResult.OK) project.Text = dialog.SelectedPath;
        }
    }

    async Task Generate()
    {
        if (busy) return;
        open.Enabled = false;
        details.Clear();
        string path = project.Text.Trim().Trim('"');
        if (!Directory.Exists(path))
        {
            status.Text = "Selecciona una carpeta de proyecto válida con Examinar.";
            project.Focus();
            return;
        }
        string appFolder = Path.GetDirectoryName(typeof(MainForm).Assembly.Location);
        string engine = Path.Combine(appFolder, "jocarsa-documentacion.exe");
        if (!File.Exists(engine))
        {
            status.Text = "Falta el generador. Extrae todos los archivos del ZIP en la misma carpeta.";
            return;
        }
        busy = true;
        generate.Enabled = browse.Enabled = project.Enabled = false;
        status.Text = "Generando el informe…";
        UseWaitCursor = true;
        try
        {
            path = Path.GetFullPath(path);
            outputFolder = Path.Combine(path, "documentacion");
            var start = new ProcessStartInfo(engine) {
                Arguments = Quote(path) + " " + Quote(outputFolder),
                WorkingDirectory = appFolder,
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                StandardOutputEncoding = Encoding.UTF8,
                StandardErrorEncoding = Encoding.UTF8
            };
            using (var process = new Process { StartInfo = start })
            {
                process.Start();
                Task<string> stdout = process.StandardOutput.ReadToEndAsync();
                Task<string> stderr = process.StandardError.ReadToEndAsync();
                await Task.Run(() => process.WaitForExit());
                details.Text = ((await stdout) + (await stderr)).Replace("\r\n", "\n").Replace("\n", Environment.NewLine);
                if (process.ExitCode == 0)
                {
                    status.Text = "Informe generado. Pulsa Abrir carpeta del informe para encontrarlo.";
                    open.Enabled = true;
                }
                else status.Text = "No se pudo generar el informe. Consulta el detalle de abajo.";
            }
        }
        catch (Exception ex)
        {
            status.Text = "No se pudo generar el informe.";
            details.Text = ex.Message;
        }
        finally
        {
            busy = false;
            UseWaitCursor = false;
            generate.Enabled = browse.Enabled = project.Enabled = true;
        }
    }

    // Escapa argumentos siguiendo las reglas de la línea de comandos de Windows,
    // incluidas las barras finales de las raíces de unidades.
    static string Quote(string value)
    {
        var result = new StringBuilder("\"");
        int slashes = 0;
        foreach (char c in value)
        {
            if (c == '\\') { slashes++; continue; }
            if (c == '"') { result.Append('\\', slashes * 2 + 1); result.Append(c); }
            else { result.Append('\\', slashes); result.Append(c); }
            slashes = 0;
        }
        result.Append('\\', slashes * 2);
        return result.Append('"').ToString();
    }

    [STAThread]
    public static void Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new MainForm());
    }
}

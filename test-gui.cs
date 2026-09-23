using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Threading;
using System.Windows.Forms;

class GuiTests
{
    static void Check(bool ok, string message) { if (!ok) throw new Exception(message); }
    static void Generate(MainForm form)
    {
        var button = (Button)form.Controls["Generate"];
        button.PerformClick();
        var timer = Stopwatch.StartNew();
        while (!button.Enabled && timer.ElapsedMilliseconds < 20000)
        {
            Application.DoEvents();
            Thread.Sleep(20);
        }
        Check(button.Enabled, "La generación no terminó");
    }
    [STAThread]
    static int Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        string root = Path.GetFullPath("test-output");
        string project = Path.Combine(root, "Proyecto interfaz ñ 漢字 " + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(project);
        File.WriteAllText(Path.Combine(project, "ejemplo.py"), "print('Hola desde la ventana')");
        using (var form = new MainForm())
        {
            form.StartPosition = FormStartPosition.Manual;
            form.Location = new Point(-2000, -2000);
            form.Show();
            Application.DoEvents();
            Check(form.Controls["Browse"].Text == "Examinar…", "Falta Examinar");
            Generate(form);
            Check(form.Controls["Status"].Text.Contains("válida"), "No valida la carpeta vacía");
            form.Controls["ProjectPath"].Text = project;
            Generate(form);
            Check(form.Controls["OpenOutput"].Enabled, form.Controls["Details"].Text);
            var reports = Directory.GetFiles(Path.Combine(project, "documentacion"), "*.md");
            Check(reports.Length == 1, "Falta el informe");
            Check(File.ReadAllText(reports[0]).Contains("Hola desde la ventana"), "Contenido incompleto");
            using (var bitmap = new Bitmap(form.Width, form.Height))
            {
                form.DrawToBitmap(bitmap, new Rectangle(0, 0, form.Width, form.Height));
                bitmap.Save("build/interfaz-windows.png");
            }
            File.WriteAllText(Path.Combine(project, ".jocarsa-documentacion-exclude"), "");
            Generate(form);
            Check(!form.Controls["OpenOutput"].Enabled, "Permite abrir tras error");
            Check(form.Controls["Status"].Text.Contains("No se pudo"), "No muestra el error del motor");
            Check(form.Controls["Details"].Text.Contains("excluida"), "Falta detalle del error");
            form.Controls["ProjectPath"].Text = Path.Combine(project, "no-existe");
            Generate(form);
            Check(form.Controls["Status"].Text.Contains("válida"), "No valida carpeta inexistente");
            form.Close();
        }
        Console.WriteLine("OK: interfaz, validación, generación real con Unicode, resultado y error del motor.");
        return 0;
    }
}

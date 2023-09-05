namespace pv
{
    internal class pv_main
    {
        static void Main(string[] args)
        {
            var arguments = string.Join( ",", args.ToArray());
            Console.WriteLine("Hello, World!, '" + arguments + "'.");
        }
    }
}